#include "Server.h"

bool g_shutdownLock = true;

void Monitor()
{
	if (_kbhit())
	{
		char ch = _getch();

		switch (ch)
		{
			case 'p':
			{
				PRO_PRINT("Profile.txt");
				break;
			}
			case 'q':
			{
			
				break;
			}
			case 'l':
			{
				g_shutdownLock = true;
			}
			case 'u':
			{
				g_shutdownLock = false;
			}
		}
	}
}

int wmain()
{
	Server* server = new Server;
	int prevTime = timeGetTime();

	while(server->_shutdown)
	{
		if (server->UpdateNetwork() == false)
			break;

		if (server->UpdateContents() == false)
			break;

		// Monitor
		if (timeGetTime() - prevTime > 1000)
		{
			prevTime = timeGetTime();

			cout << "Update Frame per sec  : " << server->_updateCnt << "\n";
			cout << "Network Frame per sec : " << server->_netCnt / 8 << "\n";
			cout << "Session Count : " << server->_sessionMap.size() << "\n";
			cout << "Character Count : " << server->_characterMap.size() << "\n";
			cout << "\n";
			cout << "Press 's' to profiler print \n";
			cout << "Press 'q' to shutDown \n";
			cout << "Press 'u' to unLock \n";
			cout << "Press 'l' to lock \n";
			
			if (g_shutdownLock)
			{
				cout << "Mode : Lock mode \n";
			}
			else
			{
				cout << "Mode : Unlock mode \n";
			}

			cout << "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n";

			server->_updateCnt = 0;
			server->_netCnt = 0;
		}

		// Input
		Monitor();
	}
	delete server;

	return 0;
}
