= UEFIアプリケーションのロード・実行
EFI_BOOT_SERVICES内のLoadImage()とStartImage()を使用することで、UEFIアプリケーションをロード・実行できます。ただし、そのためにはUEFIの「デバイスパス」という概念でパスを作成する必要があります。

この章ではデバイスパスを見てみる、作ってみるところから、順を追って説明し、UEFIアプリケーションをロード・実行する方法を紹介します。

なお実は、今のLinuxカーネルはUEFIアプリケーションとしてカーネルイメージを生成する機能があります。そこで、この章の最後では、Linuxカーネルの起動を行ってみます。

== 自分自身のパスを表示してみる
LoadImage()でUEFIアプリケーションの実行バイナリをロードするには、実行バイナリへのパスを「デバイスパス」というもので作成する必要があります。

実は、自分自身(EFI/BOOT/BOOTX64.EFI)のデバイスパスを取得する方法がありますので、この章では、既存のデバイスパスを改造して、起動したいUEFIアプリケーションのデバイスパスを作ることにします。

この節では、まず、自分自身のデバイスパスを画面へ表示し、どんなものか見てみます。

サンプルのディレクトリは"030_loaded_image_protocol_file_path"です。

=== EFI_LOADED_IMAGE_PROTOCOL
ロード済みのイメージ(UEFIアプリケーション)の情報を取得するには、EFI_LOADED_IMAGE_PROTOCOLを使用します(@<list>{load_start_image_loaded_image_protocol})。

//list[load_start_image_loaded_image_protocol][EFI_LOADED_IMAGE_PROTOCOLの定義(efi.hより)][c]{
struct EFI_LOADED_IMAGE_PROTOCOL {
	unsigned int Revision;
	void *ParentHandle;
	struct EFI_SYSTEM_TABLE *SystemTable;
	// Source location of the image
	void *DeviceHandle;
	struct EFI_DEVICE_PATH_PROTOCOL *FilePath;
	void *Reserved;
	// Image’s load options
	unsigned int LoadOptionsSize;
	void *LoadOptions;
	// Location where image was loaded
	void *ImageBase;
	unsigned long long ImageSize;
	enum EFI_MEMORY_TYPE ImageCodeType;
	enum EFI_MEMORY_TYPE ImageDataType;
	unsigned long long (*Unload)(void *ImageHandle);
};
//}

EFI_LOADED_IMAGE_PROTOCOLは、これまで見てきたプロトコルとは異なり、メンバのほとんどが変数や構造体のポインタで、これらのメンバにロード済みイメージの各種情報が格納されています。@<list>{load_start_image_loaded_image_protocol}を見てみると、"FilePath"というメンバがあります。しかも型が"EFI_DEVICE_PATH_PROTOCOL"という名前なので、ここにロード済みイメージのデバイスパスが格納されていそうです。

#@# なお、"EFI_DEVICE_PATH_PROTOCOL"の定義は@<list>{lip_open_protocol_efi_device_path_protocol}の通りです。

#@# //list[lip_open_protocol_efi_device_path_protocol][EFI_DEVICE_PATH_PROTOCOLの定義(efi.hより)][c]{
#@# struct EFI_DEVICE_PATH_PROTOCOL {
#@# 	unsigned char Type;
#@# 	unsigned char SubType;
#@# 	unsigned char Length[2];
#@# };
#@# //}

#@# @<list>{lip_open_protocol_efi_device_path_protocol}を見ると、パスを表すにはあまりにもメンバが少ないです。EFI_DEVICE_PATH_PROTOCOLを使用してどのようにデバイスパスを表すかは次の節で解説します。

=== OpenProtocol()でEFI_LOADED_IMAGE_PROTOCOLを取得
そして、ロード済みイメージのEFI_LOADED_IMAGE_PROTOCOLを取得するためにEFI_BOOT_SERVICESのOpenProtocol()を使用します(@<list>{load_start_image_open_protocol})。

//list[load_start_image_open_protocol][OpenProtocol()の定義(efi.hより)][c]{
struct EFI_SYSTEM_TABLE {
	・・・
	struct EFI_BOOT_SERVICES {
		・・・
		unsigned long long (*OpenProtocol)(
			void *Handle,
				/* オープンするプロトコルで
				 * 扱う対象のハンドルを指定 */
			struct EFI_GUID *Protocol,	/* プロトコルのGUID */
			void **Interface,
				/* プロトコル構造体のポインタを格納 */
			void *AgentHandle,
				/* OpenProtocol()を実行している
				 * UEFIアプリケーションのイメージハンドル
				 * すなわち、自分自身のイメージハンドル */
			void *ControllerHandle,
				/* OpenProtocol()を実行しているのが
				 * 「UEFIドライバー」である場合に指定する
				 * コントローラハンドル
				 * そうでないUEFIアプリケーションの場合、
				 * NULLを指定 */
			unsigned int Attributes
				/* プロトコルインタフェースを開くモードを指定 */
			);
		・・・
	} *BootServices;
};
//}

EFI_LOADED_IMAGE_PROTOCOLを開く場合、OpenProtocol()の第1引数へはEFI_LOADED_IMAGE_PROTOCOLで情報を見たい対象のイメージハンドルを指定し、第4引数へはOpenProtocol()を実行しているUEFIアプリケーションのイメージハンドルを指定します。今回の場合はどちらも、起動時から実行しているUEFIアプリケーションです。

なお通常、UEFIアプリケーションのイメージハンドルは後述するLoadImage()でUEFIアプリケーションをロードする際に取得しますが、起動時から実行しているUEFIアプリケーションについては、エントリ関数の第1引数"ImageHandle"が自分自身のイメージハンドルです。そのため、OpenProtocol()の第1引数と第4引数へはImageHandleを指定します。

また、OpenProtocol()の第6引数"Attributes"へ指定できるモード定数は@<list>{lsi_open_protocol_attributes}の通りです。今回の場合、"EFI_OPEN_PROTOCOL_GET_PROTOCOL"を指定します。

//list[lsi_open_protocol_attributes][OpenProtocol()第4引数"Atributes"へ指定できる定数(efi.hより)][c]{
#define EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL	0x00000001
#define EFI_OPEN_PROTOCOL_GET_PROTOCOL	0x00000002
#define EFI_OPEN_PROTOCOL_TEST_PROTOCOL	0x00000004
#define EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER	0x00000008
#define EFI_OPEN_PROTOCOL_BY_DRIVER	0x00000010
#define EFI_OPEN_PROTOCOL_EXCLUSIVE	0x00000020
//}

ここまでをまとめると、OpenProtocol()は@<list>{lsi_open_protocol_sample}の様に使用します。なお、EFI_LOADED_IMAGE_PROTOCOLのGUID"lip_guid"は、efi.h(@<list>{lsi_open_protocol_efi_h})とefi.c(@<list>{lsi_open_protocol_efi_c})へ定義を追加しています。

//listnum[lsi_open_protocol_sample][OpenProtocol()の使用例(main.cより)][c]{
#include "efi.h"
#include "common.h"

void efi_main(void *ImageHandle, struct EFI_SYSTEM_TABLE *SystemTable)
{
	unsigned long long status;
	struct EFI_LOADED_IMAGE_PROTOCOL *lip;

	efi_init(SystemTable);
	ST->ConOut->ClearScreen(ST->ConOut);

	status = ST->BootServices->OpenProtocol(
		ImageHandle, &lip_guid, (void **)&lip, ImageHandle, NULL,
		EFI_OPEN_PROTOCOL_GET_PROTOCOL);
	assert(status, L"OpenProtocol");

	while (TRUE);
}
//}

//listnum[lsi_open_protocol_efi_h][lip_guidのextern(efi.hより)][c]{
extern struct EFI_GUID lip_guid;
//}

//listnum[lsi_open_protocol_efi_c][lip_guidの定義(efi.cより)][c]{
struct EFI_GUID lip_guid = {0x5b1b31a1, 0x9562, 0x11d2,
			    {0x8e, 0x3f, 0x00, 0xa0,
			     0xc9, 0x69, 0x72, 0x3b}};
//}

=== EFI_DEVICE_PATH_TO_TEXT_PROTOCOL
EFI_DEVICE_PATH_TO_TEXT_PROTOCOLを使用することで、デバイスパスを画面に表示できるようテキストへ変換できます(@<list>{lip_efi_device_path_to_text_protocol})。

//list[lip_efi_device_path_to_text_protocol][EFI_DEVICE_PATH_TO_TEXT_PROTOCOLの定義][c]{
struct EFI_DEVICE_PATH_TO_TEXT_PROTOCOL {
	unsigned long long _buf;
	unsigned short *(*ConvertDevicePathToText)(
		const struct EFI_DEVICE_PATH_PROTOCOL* DeviceNode,
		unsigned char DisplayOnly,
		unsigned char AllowShortcuts);
};
//}

前節で取得したEFI_LOADED_IMAGE_PROTOCOLのstruct EFI_DEVICE_PATH_PROTOCOL *FilePathの内容をテキストへ変換して表示してみます(@<list>{lip_convert_device_path_to_text_sample})。

//listnum[lip_convert_device_path_to_text_sample][ConvertDevicePathToText()の使用例(main.cより)][c]{
#include "efi.h"
#include "common.h"

void efi_main(void *ImageHandle, struct EFI_SYSTEM_TABLE *SystemTable)
{
	unsigned long long status;
	struct EFI_LOADED_IMAGE_PROTOCOL *lip;

	efi_init(SystemTable);
	ST->ConOut->ClearScreen(ST->ConOut);

	status = ST->BootServices->OpenProtocol(
		ImageHandle, &lip_guid, (void **)&lip, ImageHandle, NULL,
		EFI_OPEN_PROTOCOL_GET_PROTOCOL);
	assert(status, L"OpenProtocol");

	/* 追加(ここから) */
	puts(L"lip->FilePath: ");
	puts(DPTTP->ConvertDevicePathToText(lip->FilePath, FALSE, FALSE));
	puts(L"\r\n");
	/* 追加(ここまで) */

	while (TRUE);
}
//}

実行すると@<img>{lip_convert_device_path_to_text_img}の様に表示されます。

//image[lip_convert_device_path_to_text_img][デバイスパスの表示例]{
//}

@<img>{lip_convert_device_path_to_text_img}を見ると、起動時から実行している自分自身のデバイスパスとしては、実行バイナリを配置した通り、"\EFI\BOOT\BOOTX64.EFI"というデバイスパスが格納されていることが分かります。

== デバイスパスを作成してみる(その1)
前節では確認のためにデバイスパスをテキストへ変換する関数ConvertDevicePathToText()を紹介しました。

実はその逆に、テキストをデバイスパスへ変換する関数もあります。この節ではその関数を使用してみます。

サンプルのディレクトリは"031_create_devpath_1"です。

テキストをデバイスパスへ変換する関数は、EFI_DEVICE_PATH_FROM_TEXT_PROTOCOLのConvertTextToDevicePath()です(@<list>{lip_efi_device_path_from_text})。

//list[lip_efi_device_path_from_text][EFI_DEVICE_PATH_FROM_TEXT_PROTOCOLの定義(efi.hより)][c]{
struct EFI_DEVICE_PATH_FROM_TEXT_PROTOCOL {
	unsigned long long _buf;
	struct EFI_DEVICE_PATH_PROTOCOL *(*ConvertTextToDevicePath) (
		const unsigned short *TextDevicePath);
};
//}

EFI_DEVICE_PATH_FROM_TEXT_PROTOCOLの取得はLocateProtocol()で行います(@<list>{lip_get_dpft})。

//list[lip_get_dpft][EFI_DEVICE_PATH_FROM_TEXT_PROTOCOLの取得(efi.hより)][c]{
struct EFI_DEVICE_PATH_FROM_TEXT_PROTOCOL *DPFTP;  /* => efi.hでextern */

void efi_init(struct EFI_SYSTEM_TABLE *SystemTable)
{
	・・・
	struct EFI_GUID dpftp_guid = {0x5c99a21, 0xc70f, 0x4ad2,
				      {0x8a, 0x5f, 0x35, 0xdf,
				       0x33, 0x43, 0xf5, 0x1e}};
	・・・
	ST->BootServices->LocateProtocol(&dpftp_guid, NULL, (void **)&DPFTP);
}
//}

それでは、ConvertTextToDevicePath()を使用してデバイスパスを作成してみたいと思います。前節で、起動時から実行している自分自身のデバイスパスは"\EFI\BOOT\BOOTX64.EFI"というテキストでした。"\test.efi"というテキストをデバイスパスへ変換すれば、「USBフラッシュメモリ直下のtest.efiというUEFIアプリケーション」を表せそうです。ConvertTextToDevicePath()の使用例は@<list>{lip_dpft_sample}の通りです。

//listnum[lip_dpft_sample][ConvertTextToDevicePath()の使用例(main.cより)][c]{
#include "efi.h"
#include "common.h"

void efi_main(void *ImageHandle __attribute__ ((unused)),
	      struct EFI_SYSTEM_TABLE *SystemTable)
{
	struct EFI_DEVICE_PATH_PROTOCOL *dev_path;

	efi_init(SystemTable);
	ST->ConOut->ClearScreen(ST->ConOut);

	dev_path = DPFTP->ConvertTextToDevicePath(L"\\test.efi");
	puts(L"dev_path: ");
	puts(DPTTP->ConvertDevicePathToText(dev_path, FALSE, FALSE));
	puts(L"\r\n");

	while (TRUE);
}
//}

@<list>{lip_dpft_sample}では、確認のために、作成したデバイスパスをConvertDevicePathToText()を使用してテキストへ戻し、画面表示しています。

実行すると@<img>{lip_dpft_img}の様に表示されます。

//image[lip_dpft_img][ConvertTextToDevicePath()の実行例]{
//}

意図した通りのデバイスパスが作成できました。

== デバイスパスをロードしてみる(その1)
デバイスパスを作成できたので、LoadImage()でロードしてみます。

サンプルのディレクトリは"032_load_devpath_1"です。

LoadImage()はEFI_BOOT_SERVICES内で定義されています(@<list>{ld1_load_image})。

//list[ld1_load_image][LoadImage()の定義(efi.hより)][c]{
struct EFI_SYSTEM_TABLE {
	・・・
	struct EFI_BOOT_SERVICES {
		・・・
		//
		// Image Services
		//
		unsigned long long (*LoadImage)(
			unsigned char BootPolicy,
				/* ブートマネージャーからのブートか否かの指定
				 * 今回はブートマネージャー関係ないのでFALSE */
			void *ParentImageHandle,
				/* 呼び出し元のイメージハンドル
				 * 今回の場合、エントリ関数第1引数の
				 * ImageHandleを指定 */
			struct EFI_DEVICE_PATH_PROTOCOL *DevicePath,
				/* ロードするイメージへのパス */
			void *SourceBuffer,
				/* NULLで無ければ、ロードされるイメージの
				 * コピーを持つアドレスを示す
				 * 今回は特に使用しないためNULLを指定 */
			unsigned long long SourceSize,
				/* SourceBufferのサイズを指定(単位:バイト)
				 * SourceBufferを使用しないため0を指定 */
			void **ImageHandle
				/* 生成されたイメージハンドルを格納する
				 * ポインタ */
			);
		・・・
	} *BootServices;
};
//}

使用例は@<list>{ld1_load_image_sample}の通りです。

//listnum[ld1_load_image_sample][LoadImage()の使用例(main.cより)][c]{
#include "efi.h"
#include "common.h"

void efi_main(void *ImageHandle __attribute__ ((unused)),
	      struct EFI_SYSTEM_TABLE *SystemTable)
{
	struct EFI_DEVICE_PATH_PROTOCOL *dev_path;	/* 追加 */
	unsigned long long status;	/* 追加 */
	void *image;

	efi_init(SystemTable);
	ST->ConOut->ClearScreen(ST->ConOut);

	dev_path = DPFTP->ConvertTextToDevicePath(L"\\test.efi");
	puts(L"dev_path: ");
	puts(DPTTP->ConvertDevicePathToText(dev_path, FALSE, FALSE));
	puts(L"\r\n");

	/* 追加(ここから) */
	status = ST->BootServices->LoadImage(FALSE, ImageHandle, dev_path, NULL,
					     0, &image);
	assert(status, L"LoadImage");
	puts(L"LoadImage: Success!\r\n");
	/* 追加(ここまで) */

	while (TRUE);
}
//}

デバイスパスの生成までは前節のままで、生成したデバイスパス"dev_path"をLoadImage()に渡しています。LoadImage()に失敗した場合はassert()でログを出して止まり、成功した場合はputs()で"LoadImage: Success!"が表示されます。

@<list>{ld1_load_image_sample}を実行してみます。なお、ロードされる側の"test.efi"は、適当なUEFIアプリケーションをビルドして用意しておけばよいのですが、1点だけビルド時の注意点がありますので、後述のコラムをご覧ください。ここでは、前著"フルスクラッチで作る!UEFIベアメタルプログラミング(パート1)"のサンプルプログラム"sample1_1_hello_uefi"を使用します@<fn>{part1_sample}。そしてビルドしたefiバイナリを"test.efi"とリネームし、起動ディスクとして使用するUSBフラッシュメモリのルート直下へ配置したこととします。
//footnote[part1_sample][@<href>{https://github.com/cupnes/c92_uefi_bare_metal_programming_samples}で公開しています]

実行してみると、結果は@<img>{ld1_load_image_img}の通りです。

//image[ld1_load_image_img][LoadImage()の実行例]{
//}

失敗しています。EFI_STATUSの"0x80000000 0000000E"は、最上位の0x8が"エラーである"事を示し、下位の"0xE"が"EFI_NOT_FOUND"を示します@<fn>{about_status_code}。
//footnote[about_status_code][詳しくは仕様書の"Appendix D Status Codes"を見てみてください]

LoadImage()がイメージを見つけられなかった理由はデバイスパスが完全では無かったからです。実は、デバイスパスにはもう少し付け加えなければならない要素があり、次節から説明します。

===[column] ロードされるUEFIアプリケーションは"-shared"オプションを付けてビルド
LoadImage()でロードされるUEFIアプリケーションの実行バイナリはリロケータブルなバイナリである必要があります。

そのため、Makefileの"fs/EFI/BOOT/BOOTX64.EFI"ターゲットで"x86_64-w64-mingw32-gcc"のオプションを指定している箇所へ"-shared"オプションを追加してください(@<list>{add_shared})。

//list[add_shared]["-shared"の追加例]{
all: fs/EFI/BOOT/BOOTX64.EFI

fs/EFI/BOOT/BOOTX64.EFI: main.c
	mkdir -p fs/EFI/BOOT
	x86_64-w64-mingw32-gcc -Wall -Wextra -e efi_main -nostdinc \
	-nostdlib -fno-builtin -Wl,--subsystem,10 -shared -o $@ $<
	#                                         ^^^^^^^
	#                                          追加
・・・
//}

== デバイスを指定するパス指定
これまで、"\EFI\BOOT\BOOTX64.EFI"や"\test.efi"のようにパスを指定してみました。ただし、考えてみれば、これでは「どのデバイスであるか」を指定できておらず、例えば、ノートPC本体のハードディスクとUSBフラッシュメモリのどちらを指しているのかが分かりません。そもそも、デバイスをパスの形で指定できるからこそ"デバイスパス"であるにも関わらず、"デバイス"の箇所を指定していませんでした。

それでは、デバイスの部分はどのように指定するのかを確認するために、例として、起動時から実行しているUEFIアプリケーション自身のデバイス部分のパスを見てみます。

サンプルのディレクトリは"033_loaded_image_protocol_device_handle"です。

実は、デバイス部分のパスを得るための情報もEFI_LOADED_IMAGE_PROTOCOLにあります(@<list>{load_start_image_loaded_image_protocol_2})。

//list[load_start_image_loaded_image_protocol_2][(再掲)EFI_LOADED_IMAGE_PROTOCOLの定義(efi.hより)][c]{
struct EFI_LOADED_IMAGE_PROTOCOL {
	unsigned int Revision;
	void *ParentHandle;
	struct EFI_SYSTEM_TABLE *SystemTable;
	// Source location of the image
	void *DeviceHandle;
	struct EFI_DEVICE_PATH_PROTOCOL *FilePath;
	void *Reserved;
	// Image’s load options
	unsigned int LoadOptionsSize;
	void *LoadOptions;
	// Location where image was loaded
	void *ImageBase;
	unsigned long long ImageSize;
	enum EFI_MEMORY_TYPE ImageCodeType;
	enum EFI_MEMORY_TYPE ImageDataType;
	unsigned long long (*Unload)(void *ImageHandle);
};
//}

"Source location of the image"のコメントが書かれている箇所には"FilePath"の他に"DeviceHandle"があります。この"DeviceHandle"を使用してデバイス部分のパスを得ることができます。

デバイスパス(EFI_DEVICE_PATH_PROTOCOL)を得る方法は、EFI_LOADED_IMAGE_PROTOCOLを取得する時と同じく、OpenProtocol()を使用します。そして、OpenProtocol()の第1引数にこの"DeviceHandle"を指定することで、"DeviceHandle"の"EFI_DEVICE_PATH_PROTOCOL"を得ることができます(@<list>{get_device_path_to_devhandle_sample})。

//listnum[get_device_path_to_devhandle_sample][DeviceHandleのデバイスパスを取得する例(main.cより)][c]{
#include "efi.h"
#include "common.h"

void efi_main(void *ImageHandle, struct EFI_SYSTEM_TABLE *SystemTable)
{
	struct EFI_LOADED_IMAGE_PROTOCOL *lip;
	struct EFI_DEVICE_PATH_PROTOCOL *dev_path;
	unsigned long long status;

	efi_init(SystemTable);
	ST->ConOut->ClearScreen(ST->ConOut);

	/* ImageHandleのEFI_LOADED_IMAGE_PROTOCOL(lip)を取得 */
	status = ST->BootServices->OpenProtocol(
		ImageHandle, &lip_guid, (void **)&lip, ImageHandle, NULL,
		EFI_OPEN_PROTOCOL_GET_PROTOCOL);
	assert(status, L"OpenProtocol(lip)");

	/* lip->DeviceHandleのEFI_DEVICE_PATH_PROTOCOL(dev_path)を取得 */
	status = ST->BootServices->OpenProtocol(
		lip->DeviceHandle, &dpp_guid, (void **)&dev_path, ImageHandle,
		NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
	assert(status, L"OpenProtocol(dpp)");

	/* dev_pathをテキストへ変換し表示 */
	puts(L"dev_path: ");
	puts(DPTTP->ConvertDevicePathToText(dev_path, FALSE, FALSE));
	puts(L"\r\n");

	while (TRUE);
}
//}

なお、上記の実装に合わせて、efi.hとefi.cへEFI_DEVICE_PATH_PROTOCOLのGUIDを定義します(@<list>{dpp_guid_efi_h}、@<list>{dpp_guid_efi_c})。

//list[dpp_guid_efi_h][EFI_DEVICE_PATH_PROTOCOLのGUIDのextern(efi.hより)][c]{
extern struct EFI_GUID dpp_guid;
//}

//list[dpp_guid_efi_c][EFI_DEVICE_PATH_PROTOCOLのGUIDの定義(efi.cより)][c]{
struct EFI_GUID dpp_guid = {0x09576e91, 0x6d3f, 0x11d2,
			    {0x8e, 0x39, 0x00, 0xa0,
			     0xc9, 0x69, 0x72, 0x3b}};
//}

実行すると、@<img>{get_dpp_devpath_img}の様に表示されます。

//image[get_dpp_devpath_img][DeviceHandelのデバイスパスの表示例]{
//}

"PciRoot"から始まるテキストが表示されており、「デバイス」の「パス」っぽいですね。本書で扱う範囲では立ち入らずに済むためあまり説明しませんが、UEFIの概念ではこの様に、デバイスをパスの形で指定して扱います。ストレージデバイスだけでなく、PCに接続されるマウス等のデバイスもこの様にパスの形で指定します。

== デバイスパスを作成してみる(その2)
ここまでで、"PciRoot"から始まる正に"デバイス"を指定しているパスと、"\test.efi"といったよく見るファイルのパスの2つのパスを確認しました。実は、この2つを連結することでフルパスとなります。

サンプルのディレクトリは"034_create_devpath_2"です。

パス連結等のパスを操作する関数群を持つのがEFI_DEVICE_PATH_UTILITIES_PROTOCOLです。ここでは、AppendDeviceNode()を使用します(@<list>{dpup_append_device_node})。

//list[dpup_append_device_node][EFI_DEVICE_PATH_UTILITIES_PROTOCOL.AppendDeviceNode()の定義(efi.hより)][c]{
struct EFI_DEVICE_PATH_UTILITIES_PROTOCOL {
	unsigned long long _buf[3];
	/* デバイスパスとノードを連結し、連結結果のパスを返す */
	struct EFI_DEVICE_PATH_PROTOCOL *(*AppendDeviceNode)(
		const struct EFI_DEVICE_PATH_PROTOCOL *DevicePath,
			/* 連結するデバイスパスを指定 */
		const struct EFI_DEVICE_PATH_PROTOCOL *DeviceNode
			/* 連結するデバイスノードを指定 */
		);
};
//}

"デバイスノード"は、デバイスパスの部分で、"\"で区切られた1要素のことです。例えば、"\EFI\BOOT\BOOTX64.EFI"の場合、"EFI"や"BOOT"がデバイスノードです。そのため、AppendDeviceNode()は、デバイスパスの末尾に1つのノードを追加する関数、となります@<fn>{dpup_append_dpdp}。
//footnote[dpup_append_dpdp][EFI_DEVICE_PATH_UTILITIES_PROTOCOLの中にはデバイスパスにデバイスパスを連結する関数もあるのですが、使用しないため省略します。]

今回の場合、ファイルパスに相当する部分は"test.efi"という単一のデバイスノードで済むため、AppendDeviceNode()を使用します。それに併せて、テキストからデバイスパスを生成する関数群を持つEFI_DEVICE_PATH_UTILITIES_PROTOCOLにはConvertTextToDeviceNode()という関数もあるので、ここではこちらの関数を使用します(@<list>{dpftp_test2node})。

//list[dpftp_test2node][ConvertTextToDeviceNode()の定義(efi.hより)][c]{
struct EFI_DEVICE_PATH_FROM_TEXT_PROTOCOL {
	/* 指定されたテキストをデバイスノードへ変換、変換後のデバイスノードを返す */
	struct EFI_DEVICE_PATH_PROTOCOL *(*ConvertTextToDeviceNode) (
		const unsigned short *TextDeviceNode
			/* デバイスノードへ変換するテキストを指定 */
		);
	struct EFI_DEVICE_PATH_PROTOCOL *(*ConvertTextToDevicePath) (
		const unsigned short *TextDevicePath);
};
//}

以上を踏まえ、デバイスパスとデバイスノードを連結する例は@<list>{append_device_node_sample}の通りです。

//listnum[append_device_node_sample][AppendDeviceNode()の使用例(main.cより)][c]{
#include "efi.h"
#include "common.h"

void efi_main(void *ImageHandle, struct EFI_SYSTEM_TABLE *SystemTable)
{
	struct EFI_LOADED_IMAGE_PROTOCOL *lip;
	struct EFI_DEVICE_PATH_PROTOCOL *dev_path;
	struct EFI_DEVICE_PATH_PROTOCOL *dev_node;		/* 追加 */
	struct EFI_DEVICE_PATH_PROTOCOL *dev_path_merged;	/* 追加 */
	unsigned long long status;

	efi_init(SystemTable);
	ST->ConOut->ClearScreen(ST->ConOut);

	/* ImageHandleのEFI_LOADED_IMAGE_PROTOCOL(lip)を取得 */
	status = ST->BootServices->OpenProtocol(
		ImageHandle, &lip_guid, (void **)&lip, ImageHandle, NULL,
		EFI_OPEN_PROTOCOL_GET_PROTOCOL);
	assert(status, L"OpenProtocol(lip)");

	/* lip->DeviceHandleのEFI_DEVICE_PATH_PROTOCOL(dev_path)を取得 */
	status = ST->BootServices->OpenProtocol(
		lip->DeviceHandle, &dpp_guid, (void **)&dev_path, ImageHandle,
		NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
	assert(status, L"OpenProtocol(dpp)");

	/* 追加・変更(ここから) */
	/* "test.efi"のデバイスノードを作成 */
	dev_node = DPFTP->ConvertTextToDeviceNode(L"test.efi");

	/* dev_pathとdev_nodeを連結 */
	dev_path_merged = DPUP->AppendDeviceNode(dev_path, dev_node);

	/* dev_path_mergedをテキストへ変換し表示 */
	puts(L"dev_path_merged: ");
	puts(DPTTP->ConvertDevicePathToText(dev_path_merged, FALSE, FALSE));
	puts(L"\r\n");
	/* 追加・変更(ここまで) */

	while (TRUE);
}
//}

実行すると@<img>{append_device_node_img}の様に表示されます。

//image[append_device_node_img][AppendDeviceNode()の実行例]{
//}

===[column] デバイスパスとデバイスノードを表すEFI_DEVICE_PATH_PROTOCOLについて
デバイスパスとデバイスノードは型は分かれておらず、共にEFI_DEVICE_PATH_PROTOCOLです。

どういうことかというと、EFI_DEVICE_PATH_PROTOCOLがデバイスノードで、EFI_DEVICE_PATH_PROTOCOLが連結することでデバイスパスとなります。

それでは、EFI_DEVICE_PATH_PROTOCOLはどういう定義になっているかというと、@<list>{efi_device_path_protocol}の様になっています。

//list[efi_device_path_protocol][EFI_DEVICE_PATH_PROTOCOLの定義(efi.hより)][c]{
struct EFI_DEVICE_PATH_PROTOCOL {
	unsigned char Type;
	unsigned char SubType;
	unsigned char Length[2];
};
//}

@<list>{efi_device_path_protocol}を見ると、リンクリストを構成するようなメンバはありません。EFI_DEVICE_PATH_PROTOCOLはメモリ上に連続に並ぶことでパスを構成します。

また、@<list>{efi_device_path_protocol}を見ると、ファイルパスで必要となる"ファイル名"といった要素を格納するようなメンバがありません。実は、EFI_DEVICE_PATH_PROTOCOLは各種デバイスノードのヘッダ部分のみで、ボディに当たる部分はEFI_DEVICE_PATH_PROTOCOLに続くメモリ領域へ配置します。そのため、デバイスノードのタイプを"Type"・"SubType"メンバで指定し、ヘッダ・ボディ含めたサイズを"Length"で指定します。

デバイスパス内のノード数を示す要素はどこにもありません。ノード終端を示すデバイスノードを配置することでデバイスノードの終わりを示します。

UEFIでは、デバイスパスやノードの構造やメモリ上の配置は意識せずに使用できるよう、EFI_DEVICE_PATH_FROM_TEXT_PROTOCOLやEFI_DEVICE_PATH_UTILITIES_PROTOCOLといったプロトコルを用意しています。AppendDeviceNode()でデバイスパスへデバイスノードを追加する際も、ノード終端の要素は自動で配置してくれます。そのため、UEFIのファームウェアを仕様書通りに叩く上では特に意識する必要はありません。

== デバイスパスをロードしてみる(その2)
それでは、再びデバイスパスのロードを試してみます。

サンプルのディレクトリは"035_load_devpath_2"です。

前節のサンプル(@<list>{append_device_node_sample})へ@<list>{ld1_load_image_sample}で紹介したLoadImage()の処理を追加するだけです(@<list>{load_device_path_merged_sample})。

//listnum[load_device_path_merged_sample][フルパスのデバイスパスをロードする例(main.cより)][c]{
#include "efi.h"
#include "common.h"

void efi_main(void *ImageHandle, struct EFI_SYSTEM_TABLE *SystemTable)
{
	struct EFI_LOADED_IMAGE_PROTOCOL *lip;
	struct EFI_DEVICE_PATH_PROTOCOL *dev_path;
	struct EFI_DEVICE_PATH_PROTOCOL *dev_node;
	struct EFI_DEVICE_PATH_PROTOCOL *dev_path_merged;
	unsigned long long status;
	void *image;	/* 追加 */

	efi_init(SystemTable);
	ST->ConOut->ClearScreen(ST->ConOut);

	/* ImageHandleのEFI_LOADED_IMAGE_PROTOCOL(lip)を取得 */
	status = ST->BootServices->OpenProtocol(
		ImageHandle, &lip_guid, (void **)&lip, ImageHandle, NULL,
		EFI_OPEN_PROTOCOL_GET_PROTOCOL);
	assert(status, L"OpenProtocol(lip)");

	/* lip->DeviceHandleのEFI_DEVICE_PATH_PROTOCOL(dev_path)を取得 */
	status = ST->BootServices->OpenProtocol(
		lip->DeviceHandle, &dpp_guid, (void **)&dev_path, ImageHandle,
		NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
	assert(status, L"OpenProtocol(dpp)");

	/* "test.efi"のデバイスノードを作成 */
	dev_node = DPFTP->ConvertTextToDeviceNode(L"test.efi");

	/* dev_pathとdev_nodeを連結 */
	dev_path_merged = DPUP->AppendDeviceNode(dev_path, dev_node);

	/* dev_path_mergedをテキストへ変換し表示 */
	puts(L"dev_path_merged: ");
	puts(DPTTP->ConvertDevicePathToText(dev_path_merged, FALSE, FALSE));
	puts(L"\r\n");

	/* 追加(ここから) */
	/* dev_path_mergedをロード */
	status = ST->BootServices->LoadImage(FALSE, ImageHandle,
					     dev_path_merged, NULL, 0, &image);
	assert(status, L"LoadImage");
	puts(L"LoadImage: Success!\r\n");
	/* 追加(ここまで) */

	while (TRUE);
}
//}

実行してみると、今度はロードに成功しました(@<img>{load_device_path_merged_img})。

//image[load_device_path_merged_img][フルパスのデバイスパスをロードする実行例]{
//}

== ロードしたイメージを実行してみる
ロードが成功しましたので、いよいよ実行してみます。

サンプルのディレクトリは"036_start_devpath"です。

ロードしたイメージを実行するにはEFI_BOOT_SERVICESのStartImage()を使用します(@<list>{start_image_definition})。

//list[start_image_definition][StartImage()の定義(efi.hより)][c]{
struct EFI_SYSTEM_TABLE {
	・・・
	struct EFI_BOOT_SERVICES {
		・・・
		//
		// Image Services
		//
		unsigned long long (*LoadImage)(
			unsigned char BootPolicy,
			void *ParentImageHandle,
			struct EFI_DEVICE_PATH_PROTOCOL *DevicePath,
			void *SourceBuffer,
			unsigned long long SourceSize,
			void **ImageHandle);
		unsigned long long (*StartImage)(
			void *ImageHandle,
				/* 実行するイメージハンドル */
			unsigned long long *ExitDataSize,
				/* 第3引数ExitDataのサイズを指定
				 * ExitDataがNULLの場合、こちらもNULLを指定 */
			unsigned short **ExitData
				/* 呼びだされたイメージが
				 * EFI_BOOT_SERVICES.Exit()関数で終了した場合に
				 * 呼び出し元へ返されるデータのポインタのポインタ
				 * 使用しないため今回はNULL */
			);
		・・・
	} *BootServices;
};
//}

StartImage()の使用例は@<list>{start_image_sample}の通りです。

//listnum[start_image_sample][StartImage()の使用例(main.cより)][c]{
#include "efi.h"
#include "common.h"

void efi_main(void *ImageHandle, struct EFI_SYSTEM_TABLE *SystemTable)
{
	struct EFI_LOADED_IMAGE_PROTOCOL *lip;
	struct EFI_DEVICE_PATH_PROTOCOL *dev_path;
	struct EFI_DEVICE_PATH_PROTOCOL *dev_node;
	struct EFI_DEVICE_PATH_PROTOCOL *dev_path_merged;
	unsigned long long status;
	void *image;

	efi_init(SystemTable);
	ST->ConOut->ClearScreen(ST->ConOut);

	/* ImageHandleのEFI_LOADED_IMAGE_PROTOCOL(lip)を取得 */
	status = ST->BootServices->OpenProtocol(
		ImageHandle, &lip_guid, (void **)&lip, ImageHandle, NULL,
		EFI_OPEN_PROTOCOL_GET_PROTOCOL);
	assert(status, L"OpenProtocol(lip)");

	/* lip->DeviceHandleのEFI_DEVICE_PATH_PROTOCOL(dev_path)を取得 */
	status = ST->BootServices->OpenProtocol(
		lip->DeviceHandle, &dpp_guid, (void **)&dev_path, ImageHandle,
		NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
	assert(status, L"OpenProtocol(dpp)");

	/* "test.efi"のデバイスノードを作成 */
	dev_node = DPFTP->ConvertTextToDeviceNode(L"test.efi");

	/* dev_pathとdev_nodeを連結 */
	dev_path_merged = DPUP->AppendDeviceNode(dev_path, dev_node);

	/* dev_path_mergedをテキストへ変換し表示 */
	puts(L"dev_path_merged: ");
	puts(DPTTP->ConvertDevicePathToText(dev_path_merged, FALSE, FALSE));
	puts(L"\r\n");

	/* dev_path_mergedをロード */
	status = ST->BootServices->LoadImage(FALSE, ImageHandle,
					     dev_path_merged, NULL, 0, &image);
	assert(status, L"LoadImage");
	puts(L"LoadImage: Success!\r\n");

	/* 追加(ここから) */
	/* imageの実行を開始する */
	status = ST->BootServices->StartImage(image, NULL, NULL);
	assert(status, L"StartImage");
	puts(L"StartImage: Success!\r\n");
	/* 追加(ここまで) */

	while (TRUE);
}
//}

実行例は@<img>{start_image_img}の通りです。

//image[start_image_img][StartImage()の実行例]{
//}

無事に実行できました！これで、今後はUEFIアプリケーションをバイナリ単位で分割し、呼び出す事ができます。

そう言えば、前著(パート1)のsample1_1_hello_uefiでは、CR('\r')が入っていないので、今回のように、別のUEFIアプリケーションから呼び出された場合、改行はしますが、行頭は戻ってないですね。

== Linuxを起動してみる: カーネルビルド
現在のLinuxカーネルはビルド時の設定(menuconfig)でUEFIバイナリを生成するように設定できます。LinuxカーネルをUEFIバイナリとしてビルドすることで、Linuxのイメージ(bzImage)をこれまで説明したUEFIアプリケーションの実行方法で実行させることができます。

サンプルのディレクトリは"037_start_bzImage"です。

=== ビルド環境を準備
Debian、Ubuntu等のパッケージ管理システムでAPTが使用できる事を前提に説明します。

APTでは"@<code>{apt-get build-dep <パッケージ名>}"というコマンドで、指定したパッケージ名をビルドするために必要な環境をインストールすることができます。

カーネルイメージは"linux-image-<バージョン>-<アーキテクチャ>"というパッケージ名です。現在使用しているカーネルバージョンとアーキテクチャは"@<code>{uname -r}"コマンドで取得できるので、結果としては、以下のコマンドでLinuxカーネルをビルドする環境をインストールできます。

//cmd{
$ @<b>{sudo apt-get build-dep linux-image-$(uname -r)}
//}

なお、menuconfigを使用するための"libncurses5-dev"パッケージがbuild-depでインストールされないので、別途インストールする必要があります。

//cmd{
$ @<b>{sudo apt install libncurses5-dev}
//}

=== Linuxカーネルのソースコードを準備
ソースコードもAPTでは"@<code>{apt-get source <パッケージ名>}"というコマンドで取得できます。しかし、せっかくなのでここでは本家であるkernel.orgから最新の安定版をダウンロードして使用してみることにします。

@<href>{https://www.kernel.org/}へアクセスし、"Latest Stable Kernel:"からダウンロードしてください(@<img>{kernel_org})。

//image[kernel_org][kernel.orgのウェブサイト]{
//}

ダウンロード後は、展開しておいてください。

//cmd{
$ @<b>{tar Jxf linux-<バージョン>.tar.xz}
//}

=== ビルド設定を行う
Linuxカーネルのビルドの設定を行います。

まず、展開したLinuxカーネルのソースコードディレクトリへ移動し、x86_64向けのデフォルト設定を反映させます。

//cmd{
$ @<b>{cd linux-<バージョン>}
$ make x86_64_defconfig
//}

そして、menuconfigで、UEFIバイナリを生成する設定(コンフィグシンボル名"CONFIG_EFI_STUB")を有効化します。

menuconfigを起動させます。

//cmd{
$ @<b>{make menuconfig}
//}

すると、@<img>{menuconfig_01}の画面になります。

//image[menuconfig_01][menuconfig画面]{
//}

@<list>{to_config_efi_stub}の場所にあるCONFIG_EFI_STUBを有効化します。

//list[to_config_efi_stub][CONFIG_EFI_STUBの場所]{
Processor type and features  --->
  [*] EFI runtime service support
  [*]   EFI stub support  <== 有効化
//}

=== ビルドする
makeコマンドでビルドします。-jオプションでビルドのスレッド数を指定できます。ここではnprocコマンドで取得したCPUコア数を-jオプションに渡しています。

//cmd{
$ @<b>{make -j $(nproc)}
//}

ビルドには時間がかかりますので、のんびりと待ちましょう。

ビルドが完了すると、"bzImage"というイメージファイルができあがっています。

//cmd{
$ @<b>{ls arch/x86/boot/bzImage}
arch/x86/boot/bzImage
//}

=== 起動してみる
arch/x86/boot/bzImageをUSBフラッシュメモリ等のこれまで"test.efi"を配置していた場所に"bzImage.efi"という名前で配置し、@<list>{start_image_sample}の"test.efi"を"bzImage.efi"へ変更するだけで、Linuxカーネルを起動できます@<img>{boot_linux_fail}。

//image[boot_linux_fail][Linux起動。。ただし失敗]{
//}

Linuxカーネルの起動は始まりますが、ルートファイルシステムを何も準備していないのでカーネルパニックで止まります。

===[nonum] 補足: menuconfigでは検索が使えます
もし、前述の場所に設定項目が無い場合は、menuconfigの検索機能を使ってみてください。

menuconfig画面内で"/(スラッシュ)"キーを押下すると@<img>{menuconfig_search}の画面になります。

//image[menuconfig_search][menuconfig検索画面]{
//}

この画面で"efi_stub"の様に検索したいキーワードを入力@<fn>{about_menuconfig_search}し、Enterを押下すると、コンフィギュレーションの依存関係や設定項目の場所等の情報が表示されます(@<img>{menuconfig_search_result})。なお、検索結果の画面ではコンフィグシンボル名の"CONFIG_"は省略されています。
//footnote[about_menuconfig_search][大文字/小文字はどちらでも構いません]

//image[menuconfig_search_result][menuconfig検索結果]{
//}

@<img>{menuconfig_search_result}では、"Location"の欄にコンフィグの場所が、"Prompt"の欄に設定項目名が記載されています。

"Location"と"Prompt"が示す場所にも設定項目が無い場合は、特にコンフィグの依存関係("Depends on")を見てみると良いです。@<img>{menuconfig_search_result}の場合、"Depends on"は、"CONFIG_EFIが有効で、かつCONFIG_X86_USE_3DNOWが無効であること"を示しており、"[]"の中は現在の設定値です。現在の設定値が要求している依存関係と一致していない場合、その設定項目はmenuconfig画面内に現れませんので、先に依存するコンフィグを変更する必要があります。

== Linuxを起動してみる: カーネル起動オプション指定
前節では、Linuxカーネルが起動時に参照するルートファイルシステム("root=")等のオプションを指定していなかったため、カーネルパニックに陥ってしまっていました。そこで、カーネル起動時のオプションを設定してみます。

サンプルのディレクトリは"038_start_bzImage_options"です。

UEFIアプリケーション実行時のオプション(引数)は、EFI_LOADED_IMAGE_PROTOCOLの@<code>{unsigned int LoadOptionsSize}メンバと@<code>{void *LoadOptions}メンバへ行います(@<list>{efi_loaded_image_protocol_definition_2})。

//list[efi_loaded_image_protocol_definition_2][(再掲)EFI_LOADED_IMAGE_PROTOCOLの定義(efi.hより)][c]{
struct EFI_LOADED_IMAGE_PROTOCOL {
	unsigned int Revision;
	void *ParentHandle;
	struct EFI_SYSTEM_TABLE *SystemTable;
	// Source location of the image
	void *DeviceHandle;
	struct EFI_DEVICE_PATH_PROTOCOL *FilePath;
	void *Reserved;
	// Image’s load options
	unsigned int LoadOptionsSize;
		/* LoadOptionsメンバのサイズを指定(バイト) */
	void *LoadOptions;
		/* イメージバイナリのロードオプションへのポインタを指定 */
	// Location where image was loaded
	void *ImageBase;
	unsigned long long ImageSize;
	enum EFI_MEMORY_TYPE ImageCodeType;
	enum EFI_MEMORY_TYPE ImageDataType;
	unsigned long long (*Unload)(void *ImageHandle);
};
//}

使用例は@<list>{bzimage_options_sample}の通りです。

//listnum[bzimage_options_sample][LoadOptionsSizeとLoadOptionsの使用例(main.cより)][c]{
#include "efi.h"
#include "common.h"

void efi_main(void *ImageHandle, struct EFI_SYSTEM_TABLE *SystemTable)
{
	struct EFI_LOADED_IMAGE_PROTOCOL *lip;
	struct EFI_LOADED_IMAGE_PROTOCOL *lip_bzimage;	/* 追加 */
	struct EFI_DEVICE_PATH_PROTOCOL *dev_path;
	struct EFI_DEVICE_PATH_PROTOCOL *dev_node;
	struct EFI_DEVICE_PATH_PROTOCOL *dev_path_merged;
	unsigned long long status;
	void *image;
	unsigned short options[] = L"root=/dev/sdb2 init=/bin/sh rootwait";
								/* 追加 */

	efi_init(SystemTable);
	ST->ConOut->ClearScreen(ST->ConOut);

	/* ImageHandleのEFI_LOADED_IMAGE_PROTOCOL(lip)を取得 */
	status = ST->BootServices->OpenProtocol(
		ImageHandle, &lip_guid, (void **)&lip, ImageHandle, NULL,
		EFI_OPEN_PROTOCOL_GET_PROTOCOL);
	assert(status, L"OpenProtocol(lip)");

	/* lip->DeviceHandleのEFI_DEVICE_PATH_PROTOCOL(dev_path)を取得 */
	status = ST->BootServices->OpenProtocol(
		lip->DeviceHandle, &dpp_guid, (void **)&dev_path, ImageHandle,
		NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
	assert(status, L"OpenProtocol(dpp)");

	/* "bzImage.efi"のデバイスノードを作成 */
	dev_node = DPFTP->ConvertTextToDeviceNode(L"bzImage.efi");

	/* dev_pathとdev_nodeを連結 */
	dev_path_merged = DPUP->AppendDeviceNode(dev_path, dev_node);

	/* dev_path_mergedをテキストへ変換し表示 */
	puts(L"dev_path_merged: ");
	puts(DPTTP->ConvertDevicePathToText(dev_path_merged, FALSE, FALSE));
	puts(L"\r\n");

	/* dev_path_mergedをロード */
	status = ST->BootServices->LoadImage(FALSE, ImageHandle,
					     dev_path_merged, NULL, 0, &image);
	assert(status, L"LoadImage");
	puts(L"LoadImage: Success!\r\n");

	/* 追加(ここから) */
	/* カーネル起動オプションを設定 */
	status = ST->BootServices->OpenProtocol(
		image, &lip_guid, (void **)&lip_bzimage, ImageHandle, NULL,
		EFI_OPEN_PROTOCOL_GET_PROTOCOL);
	assert(status, L"OpenProtocol(lip_bzimage)");
	lip_bzimage->LoadOptions = options;
	lip_bzimage->LoadOptionsSize =
		(strlen(options) + 1) * sizeof(unsigned short);
	/* 追加(ここまで) */

	/* imageの実行を開始する */
	status = ST->BootServices->StartImage(image, NULL, NULL);
	assert(status, L"StartImage");
	puts(L"StartImage: Success!\r\n");

	while (TRUE);
}
//}

@<list>{bzimage_options_sample}では、カーネル起動オプションとして"root=/dev/sdb2 init=/bin/sh rootwait"を指定しています。

"root=/dev/sdb2"は、ルートファイルシステムを配置しているパーティションへのデバイスファイルの指定です。筆者が実験で使用しているPCの場合、内蔵のHDDを"sda"として認識し、接続したUSBフラッシュメモリを"sdb"として認識します。そのため、USBフラッシュメモリの第2パーティションを指定するため、"/dev/sdb2"としています。

"init=/bin/sh"は起動時にカーネルが最初に実行する実行バイナリの指定です。何も指定しない場合、"/sbin/init"が実行されますが、起動直後にシェルを立ち上げてしまおうと、"/bin/sh"を指定しています。そのため、後述しますが、USBフラッシュメモリの第2パーティションへ/bin/shを配置しておく必要があります。

"rootwait"は、Linuxカーネルがルートファイルシステムを検出するタイミングを遅らせるオプションです。USBフラッシュメモリ等の場合、デバイスが検出されるタイミングは非同期です。デバドラの検出より先にLinuxカーネルのルートファイルシステム検出を行おうとするとルートファイルシステムの検出に失敗してしまうので、ルートファイルシステムの検出を遅延させるためのオプションです。

実行例は@<img>{bzimage_options_img}の通りです。

//image[bzimage_options_img][起動オプションを指定した実行例]{
//}

晴れて、シェルを起動させることができました。

===[nonum] 補足: ルートファイルシステムの作り方
今回の場合の様に「シェルさえ動けば良い」のであれば、ルートファイルシステムの構築には"BusyBox"が便利です。BusyBoxは"組み込みLinuxのスイスアーミーナイフ"と呼ばれるもので、一つの"busybox"という実行バイナリへシンボリックリンクを張ることで、色々なコマンドが使用できるようになるというものです。

Debian、あるいはUbuntu等のAPTを使用できる環境では、"busybox-static"というパッケージをインストールすることで、必要なライブラリが静的に埋め込まれたbusyboxバイナリを取得できます。

//cmd{
$ @<b>{sudo apt install busybox-static}
・・・
$ @<b>{ls /bin/busybox}
/bin/busybox
//}

インストールすると、/bin/busyboxへbusyboxの実行バイナリが配置されます。この実行バイナリをUSBフラッシュメモリ第2パーティションへ以下の様に配置すれば良いです。("sh"は"busybox"へのシンボリックリンクです。)

//emlist[busyboxバイナリの配置]{
USBフラッシュメモリ第2パーティション
└── bin/
    ├── busybox
    └── sh -> busybox
//}

なお、基本的なコマンドはBusyBoxで事足りますが、「aptを使用したい」等の場合はルートファイルシステムの構築が面倒になってきます。

Debianから公開されているdebootstrapというコマンドを使用すると、apt等が使用できる最低限のDebianのルートファイルシステムをコマンド一つで構築できます。

//cmd{
$ sudo debootstrap <debianのバージョン> <作成先のディレクトリ>
//}

例えば、USBフラッシュメモリの第2パーティションを/mnt/storageへマウントしている時、sid(unstable)のDebianルートファイルシステムを/mnt/storageへ構築するコマンドは以下の通りです。

//cmd{
$ sudo debootstrap sid /mnt/storage
//}
