= キーボード入力
EFI_SIMPLE_TEXT_INPUT_PROTOCOLよりも少し高機能なEFI_SIMPLE_TEXT_INPUT_EX_PROTOCOLがあります。この章ではEFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL関係のTIPSを紹介します。

EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOLのGUIDと定義は@<list>{simple_text_input_ex_protocol}の通りです。

//list[simple_text_input_ex_protocol][EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOLのGUIDと定義][c]{
struct EFI_GUID stiep_guid = {0xdd9e7534, 0x7762, 0x4698, \
			      {0x8c, 0x14, 0xf5, 0x85,	  \
			       0x17, 0xa6, 0x25, 0xaa}};

struct EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL {
	/* テキスト入力デバイスをリセットする */
	unsigned long long (*Reset)(
		struct EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *This,
		unsigned char ExtendedVerification);

	/* キー入力データを取得 */
	unsigned long long (*ReadKeyStrokeEx)(
		struct EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *This,
		struct EFI_KEY_DATA *KeyData);

	/* キー入力を待つEFI_EVENT */
	void *WaitForKeyEx;

	/* キーのトグル状態を設定(ScrollLock,NumLock,CapsLock) */
	unsigned long long (*SetState)(
		struct EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *This,
		unsigned char *KeyToggleState);

	/* 特定のキー入力を通知する関数を設定 */
	unsigned long long (*RegisterKeyNotify)(
		struct EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *This,
		struct EFI_KEY_DATA *KeyData,
		unsigned long long (*KeyNotificationFunction)(
			struct EFI_KEY_DATA *KeyData),
		void **NotifyHandle);

	/* 特定のキー入力を通知する関数を解除 */
	unsigned long long (*UnregisterKeyNotify)(
		struct EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *This,
		void *NotificationHandle);
};
//}

この章ではRegisterKeyNotify()を紹介します。

== 特定のキー入力で呼び出される関数を登録する
RegisterKeyNotify()を使用すると、特定のキー入力で呼び出される関数を登録できます(@<list>{simple_text_input_ex_register_key_notify})。

サンプルのディレクトリは"020_simple_text_input_ex_register_key_notify"です。

//list[simple_text_input_ex_register_key_notify][RegisterKeyNotify()の定義(efi.hより)][c]{
struct EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL {
	・・・
	unsigned long long (*RegisterKeyNotify)(
		struct EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *This,
		struct EFI_KEY_DATA *KeyData,
			/* イベントとして扱うキー入力を指定 */
		unsigned long long (*KeyNotificationFunction)(
			struct EFI_KEY_DATA *KeyData),
			/* 通知関数 */
		void **NotifyHandle
			/* ユニークなハンドルを返す
			 * 登録解除時に使用 */
		);
	・・・
};
//}

RegisterKeyNotify()は、第2引数へイベントとして扱うキー入力を指定し、第3引数へイベント発生を通知する関数を設定します。第2引数で指定するstruct EFI_KEY_DATAの定義は@<list>{simple_text_input_ex_efi_key_data}の通りです。

//list[simple_text_input_ex_efi_key_data][struct EFI_KEY_DATAの定義(efi.hより)][c]{
struct EFI_KEY_DATA {
	/* 入力されたキーに関する指定 */
	struct EFI_INPUT_KEY {
		unsigned short ScanCode;
			/* Unicode外のキー入力時に使用。スキャンコード */
		unsigned short UnicodeChar;
			/* Unicode内のキー入力時に使用。ユニコード値 */
	} Key;
	/* キー入力時の状態に関する指定 */
	struct EFI_KEY_STATE {
		unsigned int KeyShiftState;
			/* Shiftキーの押下状態 */
		unsigned char KeyToggleState;
			/* キーボードのトグル状態
			 * (ScrollLock,NumLock,CapsLock)を指定 */
	} KeyState;
};
//}

「どういう状態で何のキーが押されたか」を指定するためにEFI_KEY_DATAにはキーボードのトグル状態なども含まれますが、今回はEFI_INPUT_KEYのみ使用し、その他は0を設定しておきます。EFI_KEY_STATEの定義について詳しくは、仕様書の"Protocols - Console Support"の"Simple Text Input Ex Protocol"の"EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL.ReadKeyStrokeEx()"を見てみてください。

RegisterKeyNotify()の使用例は@<list>{simple_text_input_ex_register_key_notify_sample}の通りです。

//listnum[simple_text_input_ex_register_key_notify_sample][RegisterKeyNotify()の使用例(main.cより)][c]{
#include "efi.h"
#include "common.h"

unsigned char is_exit = FALSE;

unsigned long long key_notice(
	struct EFI_KEY_DATA *KeyData __attribute__ ((unused)))
{
	is_exit = TRUE;

	return EFI_SUCCESS;
}

void efi_main(void *ImageHandle __attribute__ ((unused)),
	      struct EFI_SYSTEM_TABLE *SystemTable)
{
	unsigned long long status;
	struct EFI_KEY_DATA key_data = {{0, L'q'}, {0, 0}};
	void *notify_handle;

	efi_init(SystemTable);
	ST->ConOut->ClearScreen(ST->ConOut);

	puts(L"Waiting for the 'q' key input...\r\n");

	status = STIEP->RegisterKeyNotify(STIEP, &key_data, key_notice,
					  &notify_handle);
	assert(status, L"RegisterKeyNotify");

	while (!is_exit);

	puts(L"exit.\r\n");

	while (TRUE);
}
//}

@<list>{simple_text_input_ex_register_key_notify_sample}は、'q'キーの入力でefi_main()内の特定の処理を抜けるサンプルになっています。

まず、'q'キーをイベント通知対象とするため、struct EFI_KEY_DATA key_dataへはstruct EFI_INPUT_KEYのUnicodeCharへ'q'を登録し、その他は0を設定しています。

そして、RegisterKeyNotify()を使用し、'q'キー入力でkey_notice()が呼び出される様に登録しています。

key_notice()では、グローバル変数is_exitへTRUEを設定するため、'q'キー入力でefi_main()内の"while (!is_exit);"を抜けます。

実行例は@<img>{simple_text_input_ex_register_key_notify_img1}と@<img>{simple_text_input_ex_register_key_notify_img2}の通りです。

//image[simple_text_input_ex_register_key_notify_img1][RegisterKeyNotify()実行例1('q'キー入力待ち)]{
//}

//image[simple_text_input_ex_register_key_notify_img2][RegisterKeyNotify()実行例2('q'キー入力後)]{
//}

===[nonum] 補足: Unocode外のキー入力を検出するには
Unicode外のキー入力を検出するには、struct EFI_KEY_DATAのScanCodeを使用します。仕様書"12.1.2 ConsoleIn Definition"からの引用ですが、指定できるスキャンコードは@<table>{simple_text_input_ex_scan_code_1}の通りです。

//table[simple_text_input_ex_scan_code_1][スキャンコード]{
スキャンコード	説明	スキャンコード	説明
0x00	Nullスキャンコード	0x15	F11
0x01	上	0x16	F12
0x02	下	0x68	F13
0x03	右	0x69	F14
0x04	左	0x6A	F15
0x05	Home	0x6B	F16
0x06	End	0x6C	F17
0x07	Insert	0x6D	F18
0x08	Delete	0x6E	F19
0x09	Page Up	0x6F	F20
0x0a	Page Down	0x70	F21
0x0b	F1	0x71	F22
0x0c	F2	0x72	F23
0x0d	F3	0x73	F24
0x0e	F4	0x7F	ミュート
0x0f	F5	0x80	音量を上げる
0x10	F6	0x81	音量を下げる
0x11	F7	0x100	画面の明るさを上げる
0x12	F8	0x101	画面の明るさを下げる
0x13	F9	0x102	サスペンド
0x14	F10	0x103	ハイバネート
0x17	Escape	0x104	ディスプレイトグル
　	　	0x105	リカバリー
　	　	0x106	イジェクト
　	　	0x8000-0xFFFF	OEM予約領域
//}

#@# //table[simple_text_input_ex_scan_code_2][スキャンコード(特殊なキー)]{
#@# スキャンコード	説明
#@# 0x15	F11
#@# 0x16	F12
#@# 0x68	F13
#@# 0x69	F14
#@# 0x6A	F15
#@# 0x6B	F16
#@# 0x6C	F17
#@# 0x6D	F18
#@# 0x6E	F19
#@# 0x6F	F20
#@# 0x70	F21
#@# 0x71	F22
#@# 0x72	F23
#@# 0x73	F24
#@# 0x7F	ミュート
#@# 0x80	音量を上げる
#@# 0x81	音量を下げる
#@# 0x100	画面の明るさを上げる
#@# 0x101	画面の明るさを下げる
#@# 0x102	サスペンド
#@# 0x103	ハイバネート
#@# 0x104	ディスプレイトグル
#@# 0x105	リカバリー
#@# 0x106	イジェクト
#@# 0x8000-0xFFFF	OEM予約領域
#@# //}

@<table>{simple_text_input_ex_scan_code_1}から、Escapeキーを検出したい場合は、struct EFI_KEY_DATAのScanCodeへ0x17を設定しておけば良いことが分かります。

そのため、@<list>{simple_text_input_ex_register_key_notify_sample}を、「Escapeキーでwhileループを抜ける」へ変更する場合、18行目の変数"key_data"への代入処理を@<list>{simple_text_input_ex_register_key_notify_sample_2}へ変更すれば良いです。

//list[simple_text_input_ex_register_key_notify_sample_2]["ESC"キーでwhileループを抜けるよう変更][c]{
struct EFI_KEY_DATA esc_key_data = {{0x17, 0}, {0, 0}};
//}
