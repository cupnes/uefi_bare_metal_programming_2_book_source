= タイマーイベント
タイマーイベントを扱うことで、ある処理の中で時間経過を待ったり、イベント通知関数を使用することで時間経過後に非同期に処理を行わせたりすることができます。

この章ではそれぞれのやり方を紹介します。

== 時間経過を待つ
EFI_BOOT_SERVICESのCreateEvent()とSetTimer()とWaitForEvent()で時間経過を待つことができます(@<list>{et_createevent_settimer})。なお、WaitForEvent()は前著(パート1)で紹介したので説明は省略します。

サンプルのディレクトリは"040_evt_timer_blocking"です。

//list[et_createevent_settimer][CreateEvent()とSetTimer()とWaitForEvent()の定義(efi.hより)][c]{
#define EVT_TIMER				0x80000000
#define EVT_RUNTIME				0x40000000
#define EVT_NOTIFY_WAIT				0x00000100
#define EVT_NOTIFY_SIGNAL			0x00000200
#define EVT_SIGNAL_EXIT_BOOT_SERVICES		0x00000201
#define EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE	0x60000202

enum EFI_TIMER_DELAY {
	TimerCancel,	/* 設定されているトリガー時間をキャンセル */
	TimerPeriodic,	/* 現時刻からの周期的なトリガー時間を設定 */
	TimerRelative	/* 現時刻から1回のみのトリガー時間を設定 */
};

struct EFI_SYSTEM_TABLE {
	・・・
	struct EFI_BOOT_SERVICES {
		・・・
		//
		// Event & Timer Services
		//
		/* 第1引数Typeで指定したイベントを生成し、
		 * 第5引数Eventへ格納する */
		unsigned long long (*CreateEvent)(
			unsigned int Type,
				/* イベントタイプを指定
				 * 使用できる定数は上記の通り
				 * この節ではEVT_TIMERを扱う */
			unsigned long long NotifyTpl,
				/* イベントの通知関数実行時のタスク優先度レベル
				 * 詳しくは次節で説明
				 * この節では通知関数を使わない為、0指定 */
			void (*NotifyFunction)(void *Event, void *Context),
				/* イベント発生時に実行する関数(通知関数)
				 * この節では使わない為、NULL指定*/
			void *NotifyContext,
				/* 通知関数へ渡す引数を指定
				 * この節では通知関数を使わない為、NULL指定 */
			void *Event
				/* 生成されたイベントを格納するポインタ */
			);
		/* イベント発生のトリガー時間を設定する */
		unsigned long long (*SetTimer)(
			void *Event,
				/* タイマーイベントを発生させるイベントを指定
				 * CreateEvent()で作成したイベントを指定する */
			enum EFI_TIMER_DELAY Type,
				/* タイマーイベントのタイプ指定
				 * 指定できる定数は上記の通り
				 * この節ではTimerRelativeを使用 */
			unsigned long long TriggerTime
				/* 100ns単位でトリガー時間を設定
				 * 0を指定した場合、
				 * - TimerPeriodic: 毎tick@<fn>{tick}毎にイベントを発生
				 * - TimerRelative: 次tickでイベントを発生
				 * この節では1秒(10000000)を指定 */
			);
		unsigned long long (*WaitForEvent)(
			unsigned long long NumberOfEvents,
			void **Event,
			unsigned long long *Index);
		・・・
	} *BootServices;
};
//}
//footnote[tick]["tick"とはシステムで扱う時間の単位で、語源は時計の"チクタク"。例えば1ms毎のタイマー割り込みで時間を管理しているシステムならtickは1ms。UEFIの場合どうなのかはまだ分かってないです。ごめんなさい。]

使用例は@<list>{et_createevent_settimer_sample}の通りです。

//listnum[et_createevent_settimer_sample][CreateEvent()とSetTimer()とWaitForEvent()の使用例(main.cより)][c]{
#include "efi.h"
#include "common.h"

void efi_main(void *ImageHandle __attribute__ ((unused)),
	      struct EFI_SYSTEM_TABLE *SystemTable)
{
	unsigned long long status;
	void *tevent;
	void *wait_list[1];
	unsigned long long idx;

	efi_init(SystemTable);
	ST->ConOut->ClearScreen(ST->ConOut);

	/* タイマーイベントを作成し、teventへ格納 */
	status = ST->BootServices->CreateEvent(EVT_TIMER, 0, NULL, NULL,
					       &tevent);
	assert(status, L"CreateEvent");

	/* WaitForEvent()へ渡す為にイベントリストを作成 */
	wait_list[0] = tevent;

	while (TRUE) {
		/* teventへ1秒のトリガー時間を設定 */
		status = ST->BootServices->SetTimer(tevent, TimerRelative,
						    10000000);
		assert(status, L"SetTimer");

		/* イベント発生を待つ */
		status = ST->BootServices->WaitForEvent(1, wait_list, &idx);
		assert(status, L"WaitForEvent");

		/* 画面へ"wait."を出力 */
		puts(L"wait.");
	}
}
//}

@<list>{et_createevent_settimer_sample}では、SetTimer()でトリガー時間を設定し、WaitForEvent()でイベント発生を待つ事で、1秒毎に"wait."が画面出力されます。

実行例は@<img>{et_createevent_settimer_img}の通りです。

//image[et_createevent_settimer_img][CreateEvent()とSetTimer()とWaitForEvent()の実行例]{
//}

== イベント発生時に呼び出される関数を登録する
CreateEvent()では、イベント発生時に指定した関数を呼び出すように設定できます。

サンプルのディレクトリは"041_evt_timer_nonblocking"です。

前節のCreateEvent()定義の説明箇所のみを再掲します@<list>{et_createevent}。

//list[et_createevent][(再掲)CreateEvent()の定義(efi.hより)][c]{
/* 第1引数Typeで指定したイベントを生成し、
 * 第5引数Eventへ格納する */
unsigned long long (*CreateEvent)(
	unsigned int Type,
		/* イベントタイプを指定
		 * 使用できる定数は上記の通り
		 * この節ではEVT_TIMERを扱う */
	unsigned long long NotifyTpl,
		/* イベントの通知関数実行時のタスク優先度レベル
		 * 詳しくは次節で説明
		 * この節では通知関数を使わない為、0指定 */
	void (*NotifyFunction)(void *Event, void *Context),
		/* イベント発生時に実行する関数(通知関数)
		 * この節では使わない為、NULL指定*/
	void *NotifyContext,
		/* 通知関数へ渡す引数を指定
		 * この節では通知関数を使わない為、NULL指定 */
	void *Event
		/* 生成されたイベントを格納するポインタ */
	);
//}

使用例を@<list>{et_nonblocking_sample}に示します。

//listnum[et_nonblocking_sample][CreateEvent()の通知関数設定例][c]{
#include "efi.h"
#include "common.h"

void timer_callback(void *event __attribute__ ((unused)),
		    void *context __attribute__ ((unused)))
{
	puts(L"wait.");
}

void efi_main(void *ImageHandle __attribute__ ((unused)),
	      struct EFI_SYSTEM_TABLE *SystemTable)
{
	unsigned long long status;
	void *tevent;

	efi_init(SystemTable);
	ST->ConOut->ClearScreen(ST->ConOut);

	/* タイマーイベントを作成し、teventへ格納 */
	status = ST->BootServices->CreateEvent(EVT_TIMER | EVT_NOTIFY_SIGNAL,
					       TPL_CALLBACK, timer_callback,
					       NULL, &tevent);
	assert(status, L"CreateEvent");

	/* teventへ1秒の周期トリガー時間を設定 */
	status = ST->BootServices->SetTimer(tevent, TimerPeriodic,
					    10000000);
	assert(status, L"SetTimer");

	while (TRUE);
}
//}

@<list>{et_nonblocking_sample}では、SetTimer()へ周期タイマー(TimerPeriodic)を設定してみました。1秒周期でtimer_callback()が呼び出されます。

実行例は@<img>{et_nonblocking_img}の通りです。

//image[et_nonblocking_img][CreateEvent()の通知関数実行例]{
//}
