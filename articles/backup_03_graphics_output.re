= 画面描画関係のTIPS
画面描画はEFI_GRAPHICS_OUTPUT_PROTOCOLで行います。パート1(無印)では紹介しなかった使い方をいくつか紹介します。

== グラフィックスモードの情報を取得する
コンソールへの描画に関して、テキストモード同様に、フレームバッファのグラフィックモードもこれまではデフォルトのまま使用してきました。グラフィックモードを切り替えることで画面解像度を変更できます。まずは、各グラフィックモード番号が示す解像度は何かを調べてみます。

QueryMode()を使用することで、グラフィックモード番号に対する画面解像度等の情報を取得できます@<list>{graphics_output_query_mode}。

//list[graphics_output_query_mode][EFI_GRAPHICS_OUTPUT_PROTOCOL.QueryMode()の定義][c]{
#define EFI_SUCCESS	0
#define EFI_ERROR	0x8000000000000000
#define EFI_INVALID_PARAMETER	(EFI_ERROR | 2)

struct EFI_GRAPHICS_OUTPUT_PROTOCOL {
	・・・
	unsigned long long (*QueryMode)(
		struct EFI_GRAPHICS_OUTPUT_PROTOCOL *This,
		unsigned int ModeNumber,
			/* 解像度等を確認したいモード番号を指定 */
		unsigned long long *SizeOfInfo,
			/* 第3引数"info"のサイズが設定される
			 * 変数のポインタを指定 */
		struct EFI_GRAPHICS_OUTPUT_MODE_INFORMATION **Info);
			/* グラフィックモードの情報を格納する構造体
			 * EFI_GRAPHICS_OUTPUT_MODE_INFORMATION への
			 * ポインタのポインタを指定 */
	・・・
};
//}

EFI_GRAPHICS_OUTPUT_PROTOCOLのQueryMode()は、非対応のグラフィックモード番号が指定されると、EFI_INVALID_PARAMETERを返します。

使用例は@<list>{graphics_output_query_mode_sample}の通りです。

//listnum[graphics_output_query_mode_sample][EFI_GRAPHICS_OUTPUT_PROTOCOL.QueryMode()の使用例][c]{
#include "efi.h"
#include "common.h"
#include "graphics.h"

void efi_main(void *ImageHandle __attribute__ ((unused)),
	      struct EFI_SYSTEM_TABLE *SystemTable)
{
	unsigned int mode;
	unsigned long long status;
	unsigned long long sizeofinfo;
	struct EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *info;

	efi_init(SystemTable);

	for (mode = 0; mode < GOP->Mode->MaxMode; mode++) {
		status = GOP->QueryMode(GOP, mode, &sizeofinfo, &info);
		switch (status) {
		case EFI_SUCCESS:
			puth(mode, 2);
			puts(L": 0x");
			puth(info->HorizontalResolution, 8);
			puts(L" x 0x");
			puth(info->VerticalResolution, 8);
			puts(L", pf=");
			puth(info->PixelFormat, 1);
			puts(L", mask=0x");
			puth(info->PixelInformation.RedMask, 8);
			puts(L",0x");
			puth(info->PixelInformation.GreenMask, 8);
			puts(L",0x");
			puth(info->PixelInformation.BlueMask, 8);
			puts(L",0x");
			puth(info->PixelInformation.ReservedMask, 8);
			puts(L", p/sl=0x");
			puth(info->PixelsPerScanLine, 8);
			puts(L"\r\n");
			break;

		case EFI_INVALID_PARAMETER:
			puth(mode, 2);
			puts(L": invalid mode number\r\n");
			break;

		default:
			assert(status, L"QueryMode");
			break;
		}
	}

	while (TRUE);
}
//}