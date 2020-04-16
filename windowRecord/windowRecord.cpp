// windowRecord.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//


#include <string> 
#include <Windows.h>
#include <iostream>
#include <vector>
#include<fstream>
#include <map>
#include <ctime>

using namespace std;

//宏定义
#define KEY_FLAG 0x00000001   //按键消息标志
#define MOUSE_FLAG 0x00000002 //鼠标消息标志
#define POP_FLAG 0x00000004   //弹出窗口标志

//全局变量
vector<string> win_name; //存储当前屏幕前所有的主窗口名称
fstream f;  //文件读写指针 用于记录或读取操作
HWND father_win_hwnd; //当前操作句柄的根句柄
HWND last_father_win_hwnd; //上一次的根句柄
HWND global_hwnd; //选择操作的主窗口句柄
clock_t clock_start, clock_end; //用于记录每次操作时间
bool popWin_Flag = false; //当前操作窗口是否为弹出窗口标志

//函数声明
LRESULT CALLBACK mymouse(int nCode, WPARAM wParam, LPARAM lParam); //鼠标钩子回调函数
LRESULT CALLBACK mykey(int nCode, WPARAM wParam, LPARAM lParam); //键盘钩子回调函数
BOOL CALLBACK EnumWindowsProc(HWND hWnd, LPARAM lParam); //拿到屏幕所有主窗口
void recordAction(); //记录操作
void processkCode(int vcode); //回放处理按键
int count_time(clock_t start, clock_t end); //时间计算 ms为单位


int main()
{	
	int index;
	cout << "please select the function" << endl;
	cout << "1 record function" << endl;
	cout << "2 poseback function" << endl;
	cin >> index;
	//记录功能
	if (index == 1) {
		f = fstream("record.txt", ios::out);
		recordAction();
	}
	else { //回发功能
		f = fstream("record.txt", ios::in);
		int flag;
		int temp;  
		int time;
		f >> temp;

		HWND main_hq = (HWND)temp; //得到要操作的父窗口句柄
		//激活待操作窗口
		SendMessage(main_hq, WM_SYSCOMMAND, SC_MINIMIZE, 0);
		SendMessage(main_hq, WM_SYSCOMMAND, SC_RESTORE, 0);
		//读取记录指令
		while (f >> time) {
			//操作延时
			Sleep(time);
			//数据标志位 
			f >> flag;
			cout << "..." << endl;
			//按键事件
			if (flag & KEY_FLAG) {
				int h_temp, w_temp, vcode;
				f >> h_temp;//hwnd
				f >> vcode;//vkCode
				HWND hwnd = (HWND)h_temp;
				//窗口句柄不存在 返回
				if (!IsWindow(hwnd))
					continue;
				processkCode(vcode);
					
			}
			//鼠标事件
			else if (flag & MOUSE_FLAG) {
				int h_temp, w_temp, x_shift, y_shift, zDelta;
				f >> h_temp;//hwnd
				f >> w_temp;//wParam
				HWND hwnd = (HWND)h_temp;
				//为弹出框事件 则根据父窗口句柄获取弹出框句柄
				if (flag & POP_FLAG) {
					hwnd = GetWindow(hwnd, GW_ENABLEDPOPUP);
				}

					
				WPARAM wParam = w_temp;

				if (w_temp == WM_MOUSEWHEEL) {//滚轮滚动
					f >> zDelta;
					if (!IsWindow(hwnd)) {
						continue;
					}
					RECT rect;
					GetWindowRect(hwnd, &rect);
					//SendMessage(hwnd,WM_MOUSEWHEEL, zDelta, 0);
					mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_WHEEL, (rect.left+rect.right)/2, 
						(rect.top + rect.bottom) / 2, zDelta, 0);
				}
				else {
					//根据句柄窗口所在位置和偏移量计算实际坐标
					f >> x_shift;//x_shift
					f >> y_shift;//y_shift
					if (!IsWindow(hwnd)) {
						continue;
					}
					RECT rect;
					GetWindowRect(hwnd, &rect);
					int dX = (int)(rect.left + x_shift);
					int dY = (int)(rect.top + y_shift);
					//设置鼠标
					SetCursorPos(dX, dY);
					if (w_temp == WM_LBUTTONDOWN) { //左按键按下
						//模拟左击事件
						mouse_event(MOUSEEVENTF_LEFTDOWN, dX, dY, 0, 0);
					}
					else if (w_temp == WM_LBUTTONUP) {
						mouse_event(MOUSEEVENTF_LEFTUP, dX, dY, 0, 0);
					}
					else if (w_temp == WM_RBUTTONDOWN) {
						mouse_event(MOUSEEVENTF_RIGHTDOWN, dX, dY, 0, 0);
					}
					else if (w_temp == WM_RBUTTONUP) {
						mouse_event(MOUSEEVENTF_RIGHTUP, dX, dY, 0, 0);
					}

				}
					
			}

		}
	}
	return 0;
}


//记录功能
void recordAction() {
	//获得当前桌面所有父窗口
	EnumWindows(EnumWindowsProc, NULL);
	cout << "please select the window" << endl;
	int i = 1;
	for (vector<string>::iterator iter = win_name.begin(); iter != win_name.end(); iter++) {
		cout << i++ << *iter << endl;
	}
	int index;
	cin >> index;
	cout << "the record window name is: " << win_name[index - 1] << endl;

	char* select_name = (char*)win_name[index - 1].c_str();

	LPCSTR lps = TEXT(select_name);
	HWND hq = FindWindow(NULL, lps);
	f << (int)hq << endl;
	//选中的主窗口置于最前
	SendMessage(hq, WM_SYSCOMMAND, SC_MINIMIZE, 0);
	SendMessage(hq, WM_SYSCOMMAND, SC_RESTORE, 0);
	if (hq == NULL) {
		cout << "get fail" << endl;
	}
	
	father_win_hwnd = hq;
	global_hwnd = hq;
	//注册钩子
	HHOOK select_mouse_hook = ::SetWindowsHookEx(WH_MOUSE_LL, mymouse, 0, 0);
	HHOOK select_key_hook = ::SetWindowsHookEx(WH_KEYBOARD_LL, mykey, 0, 0);
	if (select_mouse_hook == NULL) {
		cout << "register fail" << endl;
	}
	if (select_key_hook == NULL) {
		cout << "register fail" << endl;
	}
	clock_start = clock();
	MSG msg;
	//开始监控
	while (GetMessage(&msg, NULL, NULL, NULL))
	{

	}
}

//鼠标钩子回调函数
LRESULT CALLBACK mymouse(int nCode, WPARAM wParam, LPARAM lParam)
{
	//标识量
	int key_flag = 0;

	if (nCode >= 0)
	{
		//只处理左单击 右单击 滚轮滚动和 鼠标移动事件
		if (wParam == WM_LBUTTONDOWN || wParam == WM_RBUTTONDOWN || wParam == WM_MOUSEWHEEL || wParam == WM_MOUSEMOVE ||
			wParam == WM_LBUTTONUP || wParam == WM_RBUTTONUP) {
			clock_end = clock();
			//该消息为鼠标消息
			key_flag |= MOUSE_FLAG;
			PMSLLHOOKSTRUCT k = (PMSLLHOOKSTRUCT)(lParam);
			//获得当前鼠标所指向的窗口句柄
			POINT curpos;
			curpos = k->pt;
			HWND hwnd = WindowFromPoint(curpos);
			//对于鼠标移动操作 不更新父句柄
			if (wParam != WM_MOUSEMOVE) {
				//判断是否存在弹出框
				HWND temp_hwnd = GetWindow(global_hwnd, GW_ENABLEDPOPUP);
				if (temp_hwnd != global_hwnd && temp_hwnd != 0 && popWin_Flag == false) {
					popWin_Flag = true;
				}
				//判断当前父句柄是否还存在，如不存在则要更新父句柄为之前父句柄 用于处理弹出框被关闭的情况
				if (!IsWindow(father_win_hwnd)) {
					father_win_hwnd = last_father_win_hwnd;
					popWin_Flag = false;
				}
				
				//获得当前句柄的根句柄，如果跟父句柄不一致，则更新
				temp_hwnd = GetAncestor(hwnd, GA_ROOT);
				if (father_win_hwnd != temp_hwnd && temp_hwnd != 0) {
					last_father_win_hwnd = father_win_hwnd;
					father_win_hwnd = temp_hwnd;
					cout << "更新 " << last_father_win_hwnd << " " << father_win_hwnd << endl;
				}
				
				//标志该消息为弹出框消息
				if (popWin_Flag) {
					key_flag |= POP_FLAG;
				}
			}
			//得到两次事件间隔时间
			int time = count_time(clock_start, clock_end);
			//鼠标滚轮滚动事件
			if (wParam == WM_MOUSEWHEEL) {
				//得到滚动量 120 或 -120
				int zDelta = GET_WHEEL_DELTA_WPARAM(k->mouseData);
				//弹出框每次句柄都会变，因此需要判断，如果当前事件在弹出框上进行，则返回弹出框的父窗口句柄
				//消息格式 时间 事件标志 窗口句柄 事件 偏移量
				if(!popWin_Flag)
					f << time << " " << key_flag << " " << (int)hwnd << " " << wParam << " " << zDelta << endl;
				else
					f << time << " " << key_flag << " " << (int)global_hwnd << " " << wParam << " " << zDelta << endl;
			}
			else { //左右单击事件和鼠标移动事件

				//根据父句柄的位置来计算相对偏移量
				RECT rect;
				GetWindowRect(father_win_hwnd, &rect);
				int x_shift = curpos.x - rect.left;
				int y_shift = curpos.y - rect.top;

				//先判断是否为弹出框事件 
				//消息格式 时间 事件标志 窗口句柄 事件 x偏移量 y偏移量
				if(!popWin_Flag)
					f << time << " " << key_flag << " " << (int)father_win_hwnd << " " << wParam << " " << x_shift << " " << y_shift << endl;
				else
					f << time << " " << key_flag << " " << (int)global_hwnd << " " << wParam << " " << x_shift << " " << y_shift << endl;
			}

			clock_start = clock();
		}
		
	}

	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

//键盘钩子回调函数
LRESULT CALLBACK mykey(int nCode, WPARAM wParam, LPARAM lParam) {

	//获得参数列表
	PKBDLLHOOKSTRUCT p = (PKBDLLHOOKSTRUCT)lParam;
	
	if (nCode >= 0)
	{	
		//记录操作时间
		clock_end = clock();
		int time = count_time(clock_start, clock_end);
		//正常按键操作
		if ((wParam >= 0x08) && (wParam <= 0x100))
		{	
			//获取当前指针对于的窗口句柄
			POINT curpos;
			GetCursorPos(&curpos);
			HWND hwnd = WindowFromPoint(curpos);
			//存储格式为 time 按键标志 窗口句柄  按键码
			f << time << " " << KEY_FLAG << " " << (int)hwnd  << " " << p->vkCode << endl;
		}
		clock_start = clock();
	}
	//消息传递下去
	return CallNextHookEx(NULL, nCode, wParam, lParam);

}

//根据按键码进行操作
void processkCode(int vcode) {
	static bool Cap_Flag = false, Shift_Flag = false, Ctrl_Flag;
	
	if (vcode == VK_CAPITAL) { //大写键按下
		Cap_Flag = !Cap_Flag;
	}
	else if (vcode == VK_LSHIFT || vcode == VK_RSHIFT){ //shift键按下
		Shift_Flag = true;
	}
	else if (vcode == VK_LCONTROL || vcode== VK_RCONTROL) { //ctrl按下
		Ctrl_Flag = true;
	}
	else if (Ctrl_Flag == true) { //ctrl + xx 功能组合键
		Ctrl_Flag = false;
		keybd_event(17, 0, 0, 0);
		keybd_event(vcode, 0, 0, 0);
		keybd_event(vcode, 0, KEYEVENTF_KEYUP, 0);
		keybd_event(17, 0, KEYEVENTF_KEYUP, 0);
	}
	else if ((vcode >= 0x30 && vcode <= 0x39) || (vcode >= 0x41 && vcode <= 0x5A) || 
		((vcode >= 0xBA && vcode <= 0xC0) || (vcode >= 0xDB && vcode <= 0xDE))) { //数字0-9 或者shift 0-9
		if (Shift_Flag) {  //shift ++ xx组合键
			Shift_Flag = false;
			keybd_event(0xA0, 0, 0, 0);
			keybd_event(vcode, 0, 0, 0);
			keybd_event(vcode, 0, KEYEVENTF_KEYUP, 0);
			keybd_event(0xA0, 0, KEYEVENTF_KEYUP, 0);
			cout << "d" << endl;
		}
		else {
			keybd_event(vcode, 0, 0, 0);
			keybd_event(vcode, 0, KEYEVENTF_KEYUP, 0);
			cout << "d" << endl;
		}
	}
	else{ //功能键
		keybd_event(vcode, 0, 0, 0);
		keybd_event(vcode, 0, KEYEVENTF_KEYUP, 0);
	}
	

}

//计算前后两次操作时间 单位ms
int count_time(clock_t start, clock_t end) {

	float temp = (float)(end - start) / CLOCKS_PER_SEC;
	return (int)(temp * 1000);
}


//EnumWindows回调函数，hwnd为发现的顶层窗口
BOOL CALLBACK EnumWindowsProc(HWND hWnd, LPARAM lParam)
{
	if (GetParent(hWnd) == NULL && IsWindowVisible(hWnd))  //判断是否顶层窗口并且可见
	{
		char WindowTitle[100] = { 0 };
		::GetWindowText(hWnd, WindowTitle, 100);
		win_name.push_back(WindowTitle);

	}

	return true;
}