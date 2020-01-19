= 画面描画関係のTIPS
画面描画はEFI_GRAPHICS_OUTPUT_PROTOCOLで行います。パート1(無印)では紹介しなかった使い方をいくつか紹介します。

== グラフィックスモードの情報を取得する
コンソールへの描画に関して、テキストモード同様に、フレームバッファのグラフィックモードもこれまではデフォルトのまま使用してきました。グラフィックモードを切り替えることで画面解像度を変更できます。

QueryMode()を使用することで、グラフィックモード番号に対する画面解像度等の情報を取得できます@<list>{graphics_output_query_mode}。

//list[graphics_output_query_mode][EFI_GRAPHICS_OUTPUT_PROTOCOL.QueryMode()の定義][c]{
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
