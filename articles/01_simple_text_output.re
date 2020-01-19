= コンソール出力
EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL関係のTIPSを紹介します。

== 文字色と文字の背景色を設定する
SetAttribute()を使うことで文字色と文字の背景色を設定できます(@<list>{simple_text_output_protocol_set_attribute})。

サンプルのディレクトリは"010_simple_text_output_set_attribute"です。

//list[simple_text_output_protocol_set_attribute][SetAttribute()の定義(efi.hより)][c]{
//*******************************************************
// Attributes
//*******************************************************
#define EFI_BLACK	0x00
#define EFI_BLUE	0x01
#define EFI_GREEN	0x02
#define EFI_CYAN	0x03
#define EFI_RED	0x04
#define EFI_MAGENTA	0x05
#define EFI_BROWN	0x06
#define EFI_LIGHTGRAY	0x07
#define EFI_BRIGHT	0x08
#define EFI_DARKGRAY	0x08
#define EFI_LIGHTBLUE	0x09
#define EFI_LIGHTGREEN	0x0A
#define EFI_LIGHTCYAN	0x0B
#define EFI_LIGHTRED	0x0C
#define EFI_LIGHTMAGENTA	0x0D
#define EFI_YELLOW	0x0E
#define EFI_WHITE	0x0F

#define EFI_BACKGROUND_BLACK	0x00
#define EFI_BACKGROUND_BLUE	0x10
#define EFI_BACKGROUND_GREEN	0x20
#define EFI_BACKGROUND_CYAN	0x30
#define EFI_BACKGROUND_RED	0x40
#define EFI_BACKGROUND_MAGENTA	0x50
#define EFI_BACKGROUND_BROWN	0x60
#define EFI_BACKGROUND_LIGHTGRAY	0x70

struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL {
	・・・
	unsigned long long (*SetAttribute)(
		struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
		unsigned long long Attribute
			/* 文字色と文字の背景色を設定
			 * 設定できる色は上記defineの通り */
		);
	・・・
};
//}

第2引数Attributeに文字色と背景色を1バイトの値で設定します。上位4ビットが背景色で下位4ビットが文字色です。

SetAttribute()を使用して表紙画像を画面表示するサンプルが@<list>{simple_text_output_protocol_set_attribute_sample}です。

//listnum[simple_text_output_protocol_set_attribute_sample][SetAttribute()の使用例(main.cより)][c]{
#include "efi.h"
#include "common.h"

void efi_main(void *ImageHandle __attribute__ ((unused)),
	      struct EFI_SYSTEM_TABLE *SystemTable)
{
	efi_init(SystemTable);

	puts(L" ");

	ST->ConOut->SetAttribute(ST->ConOut,
				 EFI_LIGHTGREEN | EFI_BACKGROUND_LIGHTGRAY);
	ST->ConOut->ClearScreen(ST->ConOut);

	puts(L"\r\n\r\n\r\n\r\n\r\n\r\n\r\n");
	puts(L"                              ");
	puts(L"フルスクラッチで作る!\r\n");

	ST->ConOut->SetAttribute(ST->ConOut,
				 EFI_LIGHTRED | EFI_BACKGROUND_LIGHTGRAY);
	puts(L"                           ");
	puts(L"UEFIベアメタルプログラミング\r\n");

	ST->ConOut->SetAttribute(ST->ConOut,
				 EFI_LIGHTMAGENTA | EFI_BACKGROUND_LIGHTGRAY);
	puts(L"                                    ");
	puts(L"パート2\r\n");

	ST->ConOut->SetAttribute(ST->ConOut,
				 EFI_LIGHTBLUE | EFI_BACKGROUND_LIGHTGRAY);
	puts(L"\r\n\r\n\r\n\r\n\r\n\r\n\r\n");
	puts(L"                               ");
	puts(L"大ネ申  ネ右真 著\r\n");

	ST->ConOut->SetAttribute(ST->ConOut,
				 EFI_WHITE | EFI_BACKGROUND_LIGHTGRAY);
	puts(L"\r\n\r\n\r\n\r\n\r\n\r\n\r\n");
	puts(L"                         ");
	puts(L"2017-10-22 版  ");

	ST->ConOut->SetAttribute(ST->ConOut,
				 EFI_LIGHTCYAN | EFI_BACKGROUND_LIGHTGRAY);
	puts(L"henyapente ");

	ST->ConOut->SetAttribute(ST->ConOut,
				 EFI_MAGENTA | EFI_BACKGROUND_LIGHTGRAY);
	puts(L"発 行\r\n");

	while (TRUE);
}
//}

@<list>{simple_text_output_protocol_set_attribute_sample}では、efi_init()の直後に半角スペースを1つだけ出力しています。これは、実機(筆者の場合Lenovo製)のUEFIファームウェアの挙動に合わせるためで、筆者の実機の場合、起動直後の何も画面出力していない状態のClearScreen()は無視されるようで、たとえその直前でSetAttribute()で属性を設定していたとしても画面を指定した背景色でクリアすることができませんでした。そこで、一度、何らかの画面表示を行う必要があり、SetAttribute()とClearScreen()の直前に半角スペースの出力を入れています@<fn>{setattribute_clearscreen}。
//footnote[setattribute_clearscreen][「何も画面出力を行っていないのだからClearScreen()は無視する」というファームウェア側の実装は分からなくは無いのですが、SetAttribute()で背景色を変更している場合は新しい背景色で描画しなおして欲しいですね。]

== 出力可能な文字であるか否かを判定する
UEFIでは文字はUnicodeで扱います。Unicodeの範囲としては日本語なども含みますが、UEFIのファームウェアの実装としてすべての文字をサポートしているとは限りません。

TestString()を使うことで、文字(列)が出力可能か否かを判定できます(@<list>{simple_text_output_protocol_test_string})。

サンプルのディレクトリは"011_simple_text_output_test_string"です。

//list[simple_text_output_protocol_test_string][TestString()の定義(efi.hより)][c]{
struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL {
	・・・
	unsigned long long (*TestString)(
		struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
		unsigned short *String
			/* 判定したい文字列の先頭アドレスを指定 */
		);
	・・・
};
//}

TestString()は引数に文字列の先頭アドレスを指定し、判定結果は戻り値で受け取ります。出力可能な場合はEFI_SUCCESS(=0)を、出力できない文字が含まれる場合はEFI_UNSUPPORTED(=0x80000000 00000003)を返します。

使用例は@<list>{simple_text_output_protocol_test_string_sample}の通りです。

//listnum[simple_text_output_protocol_test_string_sample][TestString()の使用例(main.cより)][c]{
#include "efi.h"
#include "common.h"

void efi_main(void *ImageHandle __attribute__ ((unused)),
	      struct EFI_SYSTEM_TABLE *SystemTable)
{
	efi_init(SystemTable);
	ST->ConOut->ClearScreen(ST->ConOut);

	/* test1 */
	if (!ST->ConOut->TestString(ST->ConOut, L"Hello"))
		puts(L"test1: success\r\n");
	else
		puts(L"test1: fail\r\n");

	/* test2 */
	if (!ST->ConOut->TestString(ST->ConOut, L"こんにちは"))
		puts(L"test2: success\r\n");
	else
		puts(L"test2: fail\r\n");

	/* test3 */
	if (!ST->ConOut->TestString(ST->ConOut, L"Hello,こんにちは"))
		puts(L"test3: success\r\n");
	else
		puts(L"test3: fail\r\n");

	while (TRUE);
}
//}

QEMU上で実行してみると、OVMFのファームウェアは日本語に対応していない為、@<img>{simple_text_output_protocol_test_string_img1}の通り、日本語を含む文字列を指定しているtest2とtest3は失敗する結果となります。なお、QEMU上で実行する際にはMakefileを一部修正する必要があります。詳しくは後述のコラムを参照してください。

//image[simple_text_output_protocol_test_string_img1][QEMUでのTestString()の実行例]{
//}

日本語表示が可能な実機で実行してみると@<img>{simple_text_output_protocol_test_string_img2}の通り、test1～test3の全てが成功します。

//image[simple_text_output_protocol_test_string_img2][実機でのTestString()の実行例]{
//}

===[column] QEMU(OVMF)では実行バイナリサイズに制限(?)
QEMU用のUEFIファームウェアであるOVMFには実行バイナリサイズに制限がある様子で、コード量が増え、実行バイナリサイズが大きくなると、同じ実行バイナリが実機では実行できるがQEMU上では実行できない(UEFIファームウェアが実行バイナリを見つけてくれない)事態に陥ります。

当シリーズのUEFIベアメタルプログラミングの対象は主に実機(Lenovo製で動作確認)であるため@<fn>{qemu_iiwake}、本書ではQEMU上で動作させる際は、Makefileを修正し、不要なソースコードをコンパイル・リンクの対象に含めないようにしています。
//footnote[qemu_iiwake][言い訳ですが。。]

例えば、@<list>{simple_text_output_protocol_test_string_sample}をQEMU上で動作させる際は、Makefileを@<list>{qemu_ovmf_column}のように修正し、makeを行って下さい。

//list[qemu_ovmf_column][QEMU(OVMF)上で実行させる際のMakefile][c]{
・・・
# fs/EFI/BOOT/BOOTX64.EFI: efi.c common.c file.c graphics.c shell.c gui.c
# ↑コメントアウトか削除
fs/EFI/BOOT/BOOTX64.EFI: efi.c common.c main.c  <= 追加
	mkdir -p fs/EFI/BOOT
	x86_64-w64-mingw32-gcc -Wall -Wextra -e efi_main -nostdinc \
	-nostdlib -fno-builtin -Wl,--subsystem,10 -o $@ $+
・・・
//}

== テキストモードの情報を取得する
コンソールへのテキスト出力に関して、これまではUEFIのファームウェアが認識するデフォルトのモード(テキストモード)で使用してきました。ただし、コンソールデバイスによってはその他のテキストモードへ切り替えることで、画面あたりの列数・行数を変更できます。

サンプルのディレクトリは"012_simple_text_output_query_mode"です。

QueryMode()でテキストモード番号に対する画面あたりの列数・行数を取得できます(@<list>{simple_text_output_protocol_query_mode})。

//list[simple_text_output_protocol_query_mode][QueryMode()の定義(efi.hより)][c]{
struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL {
	・・・
	unsigned long long (*QueryMode)(
		struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
		unsigned long long ModeNumber,
			/* 列数・行数を確認したいモード番号を指定 */
		unsigned long long *Columns,
			/* 列数を格納する変数へのポインタ */
		unsigned long long *Rows);
			/* 行数を格納する変数へのポインタ */
	・・・
};
//}

QueryMode()は、第2引数で指定されたモード番号がコンソールデバイスでサポート外の場合、EFI_UNSUPPORTED(0x80000000 00000003)を返します。

なお、コンソールデバイスで使用できるモード番号の範囲はEFI_SIMPLE_TEXT_OUTPUT_PROTOCOL->Mode->MaxModeで確認できます。EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL->Modeは、SIMPLE_TEXT_OUTPUT_MODEという構造体で、定義は@<list>{simple_text_output_protocol_query_mode_test_output_mode}の通りです。

//list[simple_text_output_protocol_query_mode_test_output_mode][SIMPLE_TEXT_OUTPUT_MODE構造体の定義(efi.hより)][c]{
#define EFI_SUCCESS	0
#define EFI_ERROR	0x8000000000000000
#define EFI_UNSUPPORTED	(EFI_ERROR | 3)

struct SIMPLE_TEXT_OUTPUT_MODE {
	int MaxMode;  /* コンソールデバイスで使用できるモード番号の最大値 */
	int Mode;  /* 現在のモード番号 */
	int Attribute;  /* 現在設定されている属性値(文字色・背景色) */
	int CursorColumn;  /* カーソル位置(列) */
	int CursorRow;  /* カーソル位置(行) */
	unsigned char CursorVisible;  /* 現在のカーソル表示設定(表示=1/非表示=0) */
};
//}

以上を踏まえ、QueryMode()の使用例は@<list>{simple_text_output_protocol_query_mode_sample}の通りです。

//listnum[simple_text_output_protocol_query_mode_sample][QueryMode()の使用例(main.cより)][c]{
#include "efi.h"
#include "common.h"

void efi_main(void *ImageHandle __attribute__ ((unused)),
	      struct EFI_SYSTEM_TABLE *SystemTable)
{
	int mode;
	unsigned long long status;
	unsigned long long col, row;

	efi_init(SystemTable);
	ST->ConOut->ClearScreen(ST->ConOut);

	for (mode = 0; mode < ST->ConOut->Mode->MaxMode; mode++) {
		status = ST->ConOut->QueryMode(ST->ConOut, mode, &col, &row);
		switch (status) {
		case EFI_SUCCESS:
			puth(mode, 2);
			puts(L": ");
			puth(col, 4);
			puts(L" x ");
			puth(row, 4);
			puts(L"\r\n");
			break;

		case EFI_UNSUPPORTED:
			puth(mode, 2);
			puts(L": unsupported\r\n");
			break;

		default:
			assert(status, L"QueryMode");
			break;
		}
	}

	while (TRUE);
}
//}

実行例は@<img>{simple_text_output_protocol_query_mode_img}の通りです。

//image[simple_text_output_protocol_query_mode_img][QueryMode()の実行例]{
//}

== テキストモードを変更する
SetMode()でテキストモードを変更できます(@<list>{simple_text_output_protocol_set_mode})。

サンプルのディレクトリは"013_simple_text_output_set_mode"です。

//list[simple_text_output_protocol_set_mode][SetMode()の定義(efi.hより)][c]{
struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL {
	・・・
	unsigned long long (*SetMode)(
		struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
		unsigned long long ModeNumber  /* テキストモード番号 */
		);
	・・・
//}

使用例は@<list>{simple_text_output_protocol_set_mode_sample}の通りです。

//listnum[simple_text_output_protocol_set_mode_sample][SetMode()の使用例(main.cより)][c]{
#include "efi.h"
#include "common.h"

void efi_main(void *ImageHandle __attribute__ ((unused)),
	      struct EFI_SYSTEM_TABLE *SystemTable)
{
	int mode;
	unsigned long long status;
	unsigned long long col, row;

	efi_init(SystemTable);
	ST->ConOut->ClearScreen(ST->ConOut);

	while (TRUE) {
		for (mode = 0; mode < ST->ConOut->Mode->MaxMode; mode++) {
			status = ST->ConOut->QueryMode(ST->ConOut, mode, &col,
						       &row);
			if (status)
				continue;

			ST->ConOut->SetMode(ST->ConOut, mode);
			ST->ConOut->ClearScreen(SystemTable->ConOut);

			puts(L"mode=");
			puth(mode, 1);
			puts(L", col=0x");
			puth(col, 2);
			puts(L", row=0x");
			puth(row, 2);
			puts(L"\r\n");
			puts(L"\r\n");
			puts(L"Hello UEFI!こんにちは、せかい!");

			getc();
		}
	}
}
//}

@<list>{simple_text_output_protocol_set_mode_sample}では、コンソールデバイスが対応しているテキストモードを順に切り替え、テキストモードの情報と簡単なテキスト("Hello UEFI!こんにちは、世界！")を表示します。表示後、getc()で待ちますので、何らかのキーを入力してください(すると、次のテキストモードへ切り替わります)。

実行例は@<img>{simple_text_output_protocol_set_mode_img1}と@<img>{simple_text_output_protocol_set_mode_img2}の通りです。テキストモードによって見た目がどう変わるのかは、ぜひご自身の実機で試してみてください。

//image[simple_text_output_protocol_set_mode_img1][SetModeの実行例:モード0(列数80(0x50),行数25(0x19))の場合]{
デフォルトのモード設定
//}

//image[simple_text_output_protocol_set_mode_img2][SetModeの実行例:モード5(列数170(0xAA),行数40(0x28))の場合]{
行数・列数最大のモード設定
//}

===[column] グラフィックモードの情報を確認し、変更するには
EFI_GRAPHICS_OUTPUT_PROTOCOLもQueryMode()とSetMode()を持っています。そのため、フレームバッファのグラフィックモードについても、QueryMode()でモード番号に対する情報(解像度)を取得し、SetMode()で指定したモード番号へ変更することができます。

参考までに、EFI_GRAPHICS_OUTPUT_PROTOCOLのQueryMode()とSetMode()の定義を@<list>{graphics_output_query_mode_set_mode}に示します。

//list[graphics_output_query_mode_set_mode][QueryMode()とSetMode()の定義][c]{
#define EFI_SUCCESS	0
#define EFI_ERROR	0x8000000000000000
#define EFI_INVALID_PARAMETER	(EFI_ERROR | 2)

struct EFI_GRAPHICS_OUTPUT_PROTOCOL {
	unsigned long long (*QueryMode)(
		struct EFI_GRAPHICS_OUTPUT_PROTOCOL *This,
		unsigned int ModeNumber,
			/* 解像度等を確認したいモード番号を指定 */
		unsigned long long *SizeOfInfo,
			/* 第3引数"info"のサイズが設定される
			 * 変数のポインタを指定 */
		struct EFI_GRAPHICS_OUTPUT_MODE_INFORMATION **Info
			/* グラフィックモードの情報を格納する構造体
			 * EFI_GRAPHICS_OUTPUT_MODE_INFORMATION への
			 * ポインタのポインタを指定 */
		);

	unsigned long long (*SetMode)(
		struct EFI_GRAPHICS_OUTPUT_PROTOCOL *This,
		unsigned int ModeNumber
			/* グラフィックモード番号 */
		);
	・・・
};
//}

なお、EFI_GRAPHICS_OUTPUT_PROTOCOLのQueryMode()は、第2引数で無効なモード番号を指定した時、EFI_INVALID_PARAMETERを返します。
