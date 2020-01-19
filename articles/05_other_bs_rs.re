= BootServicesやRuntimeServicesのその他の機能
EFI_BOOT_SERVICESや、まだ機能の紹介はしていなかったEFI_RUNTIME_SERVICESには、他にも色々な機能があります。この章ではそれらの一部を紹介します。

== メモリアロケータを使う
UEFIのファームウェアはメモリアロケータを持っていて、EFI_BOOT_SERVICESのAllocatePool()とFreePool()で利用できます(@<list>{bs_alloc_free_def})。

サンプルのディレクトリは"050_bs_malloc"です。

//list[bs_alloc_free_def][AllocatePool()とFreePool()の定義(efi.hより)][c]{
enum EFI_MEMORY_TYPE {
	・・・
	EfiLoaderData,
		/* ロードされたUEFIアプリケーションのデータと
		 * UEFIアプリケーションのデフォルトのデータアロケーション領域 */
	・・・
};

struct EFI_SYSTEM_TABLE {
	・・・
	struct EFI_BOOT_SERVICES {
		・・・
		//
		// Memory Services
		//
		unsigned long long _buf3[3];
		unsigned long long (*AllocatePool)(
			enum EFI_MEMORY_TYPE PoolType,
				/* どこのメモリプールから確保するかを指定
				 * この節では"EfiLoaderData"を指定 */
			unsigned long long Size,
				/* 確保するサイズをバイト単位で指定 */
			void **Buffer
				/* 確保した領域の先頭アドレスを格納するポインタ
				 * のポインタを指定 */
			);
		unsigned long long (*FreePool)(
			void *Buffer
				/* 開放したい領域のポインタを指定 */
			);
		・・・
	} *BootServices;
};
//}

AllocatePool()第1引数の"PoolType"について、"enum EFI_MEMORY_TYPE"には他にもメモリタイプがありますが、仕様書を読む限り、UEFIアプリケーションのデータを配置する領域は"EfiLoaderData"であるようで、この節では"EfiLoaderData"を使用しています。その他のメモリタイプについては仕様書の"6.2 Memory Allocation Services"を見てみてください。

使用例は@<list>{bs_alloc_free_sample}の通りです。

//listnum[bs_alloc_free_sample][AllocatePool()とFreePool()の使用例(main.cより)][c]{
#include "efi.h"
#include "common.h"
#include "graphics.h"

#define IMG_WIDTH	256
#define IMG_HEIGHT	256

void efi_main(void *ImageHandle __attribute__ ((unused)),
	      struct EFI_SYSTEM_TABLE *SystemTable)
{
	unsigned long long status;
	struct EFI_GRAPHICS_OUTPUT_BLT_PIXEL *img_buf, *t;
	unsigned int i, j;

	efi_init(SystemTable);
	ST->ConOut->ClearScreen(ST->ConOut);

	/* 画像バッファ用のメモリを確保 */
	status = ST->BootServices->AllocatePool(
		EfiLoaderData,
		IMG_WIDTH * IMG_HEIGHT *
		sizeof(struct EFI_GRAPHICS_OUTPUT_BLT_PIXEL),
		(void **)&img_buf);
	assert(status, L"AllocatePool");

	/* 画像を生成 */
	t = img_buf;
	for (i = 0; i < IMG_HEIGHT; i++) {
		for (j = 0; j < IMG_WIDTH; j++) {
			t->Blue = i;
			t->Green = j;
			t->Red = 0;
			t->Reserved = 255;
			t++;
		}
	}

	/* 画像描画(フレームバッファへ書き込み) */
	blt((unsigned char *)img_buf, IMG_WIDTH, IMG_HEIGHT);

	/* 確保したメモリを解放 */
	status = ST->BootServices->FreePool((void *)img_buf);
	assert(status, L"FreePool");

	while (TRUE);
}
//}

@<list>{bs_alloc_free_sample}では、255x255の画像用バッファ(img_buf)を確保してピクセルデータを配置し、blt()でフレームバッファへの書き込みを行っています。

生成している画像はX軸に青色を、Y軸に緑色を、それぞれ255階調で表示するものです。

試してみると@<img>{bs_alloc_free_img}のような感じです。印刷は白黒なので、ぜひご自身で試してみてください。

//image[bs_alloc_free_img][AllocatePool()とFreePool()の実行例]{
//}

== シャットダウンする
メモリ上で動作するだけのUEFIアプリケーションならば電源ボタンで終了しても良いのですが、EFI_RUNTIME_SERVICESのResetSystem()を使用するとPCをシャットダウンしたり、再起動したりできます。

サンプルのディレクトリは"051_rs_resetsystem"です。

EFI_RUNTIME_SERVICESもEFI_BOOT_SERVICES同様にEFI_SYSTEM_TABLEのメンバです。EFI_BOOT_SERVICESはブートローダーに対して機能を提供しているのに対し、EFI_RUNTIME_SERVICESはOS起動後も使用できるという違いがあります。

具体的な契機はEFI_BOOT_SERVICESのExitBootServices()で、この関数を呼ぶと、それ以降EFI_BOOT_SERVICESの機能は無効になりますが、EFI_RUNTIME_SERVICESの機能は引き続き使用可能です。

そして、ResetSystem()はEFI_RUNTIME_SERVICES内で@<list>{rs_resetsystem_def}の様に定義されています。

//list[rs_resetsystem_def][ResetSystem()の定義(efi.hより)][c]{
enum EFI_RESET_TYPE {
	/* 説明は仕様書("7.5.1 Reset System")の意訳です。 */
	EfiResetCold,
		/* システム全体のリセット。
		 * システム内の全ての回路を初期状態へリセットする。
		 * (追記: いわゆる再起動で、CPU以外の回路も
		 * 電気的に遮断するコールドリセット)
		 * このリセットタイプはシステム操作に対し非同期で、
		 * システムの周期的動作に関係なく行われる。 */
	EfiResetWarm,
		/* システム全体の初期化。
		 * プロセッサは初期状態へリセットされ、
		 * 保留中のサイクルは破壊されない(?)。
		 * (追記: いわゆる再起動で、CPUのみリセットするウォームリセット)
		 * もしシステムがこのリセットタイプをサポートしていないならば、
		 * EfiResetColdが実施されなければならない。 */
	EfiResetShutdown,
		/* システムがACPI G2/S5あるいはG3状態に相当する電源状態へ
		 * 遷移する(追記: いわゆるシャットダウンの状態)。
		 * もしシステムがこのリセットタイプをサポートしておらず、
		 * システムが再起動した時、EfiResetColdの振る舞いを示すべき。 */
	EfiResetPlatformSpecific
		/* システム全体のリセット。
		 * 厳密なリセットタイプは引数"ResetData"に従うEFI_GUID
		 * により定義される。
		 * プラットフォームがResetData内のEFI_GUIDを認識できないならば、
		 * サポートできるリセットタイプを選択しなければならない。
		 * プラットフォームは発生した非正常なりセットから
		 * パラメータを記録することができる(?)。 */
};

struct EFI_SYSTEM_TABLE {
	・・・
	struct EFI_RUNTIME_SERVICES {
		・・・
		//
		// Miscellaneous Services
		//
		unsigned long long _buf_rs5;
		void (*ResetSystem)(
			enum EFI_RESET_TYPE ResetType,
				/* 実施されるリセットタイプ
				 * この節ではEfiResetShutdownを使用 */
			unsigned long long ResetStatus,
				/* リセットのステータスコードを指定
				 * システムのリセットが
				 * 正常なものならばEFI_SUCCESS、
				 * 異常によるものならばエラーコードを指定
				 * この節ではEFI_SUCCESS(=0)を指定 */
			unsigned long long DataSize,
				/* ResetDataのデータサイズをバイト単位で指定
				 * この節ではResetDataは使用しないので0を指定 */
			void *ResetData
				/* ResetTypeがEfiResetCold・EfiResetWarn
				 * ・EfiResetShutdownの時、
				 * ResetStatusがEFI_SUCCESSで無いならば、
				 * ResetDataへNULL終端された文字列を指定することで、
				 * 後々、呼び出し元へリセット事由を通知できる
				 * (ようである)
				 * ResetTypeがEfiResetPlatformSpecificの時、
				 * EFI_GUIDをNULL終端文字列として指定する事で、
				 * プラットフォーム依存のリセットを行える
				 * この節ではResetStatusはEFI_SUCCESSなので、
				 * 使用しない(NULLを指定する) */
			);
	} *RuntimeServices;
};
//}

使用例は@<list>{rs_resetsystem_sample}の通りです。

//listnum[rs_resetsystem_sample][ResetSystem()の使用例(main.cより)][c]{
#include "efi.h"
#include "common.h"

void efi_main(void *ImageHandle __attribute__ ((unused)),
	      struct EFI_SYSTEM_TABLE *SystemTable)
{
	efi_init(SystemTable);
	ST->ConOut->ClearScreen(ST->ConOut);

	/* キー入力待ち */
	puts(L"何かキーを押すとシャットダウンします。。。\r\n");
	getc();

	/* シャットダウン */
	ST->RuntimeServices->ResetSystem(EfiResetShutdown, EFI_SUCCESS, 0,
					 NULL);

	while (TRUE);
}
//}

@<list>{rs_resetsystem_sample}は何かキーを入力するとシャットダウンします。

実行例は@<img>{rs_resetsystem_img}の通りです。

//image[rs_resetsystem_img][ResetSystem()の実行例]{
//}
