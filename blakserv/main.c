// Meridian 59, Copyright 1994-2012 Andrew Kirmse and Chris Kirmse.
// All rights reserved.
//
// This software is distributed under a license that is described in
// the LICENSE file that accompanies it.
//
// Meridian is a registered trademark.
/*
* main.c
*

  Blakserv is the server program for Blakston.  This is a windows application,
  so we have a WinMain.  There is a dialog box interface in interfac.c.
  
	This module starts all of our "subsystems" and calls the timer loop,
	which executes until we terminate (either by the window interface or by
	a system administrator logging in to administrator mode.
	
*/

#include "blakserv.h"

/* local function prototypes */
void MainServer(void);
void MainExitServer(void);

DWORD main_thread_id;
static bool in_main_loop = false;

#ifdef BLAK_PLATFORM_WINDOWS

int WINAPI WinMain(_In_ HINSTANCE hInstance,
                   _In_opt_ HINSTANCE hPrev_instance,
                   _In_ char *command_line,
                   _In_ int how_show)
{
   main_thread_id = GetCurrentThreadId();
   StoreInstanceData(hInstance, how_show);
   MainServer();
   return 0;
}

#else

static bool interactive_mode = false;

static void Daemonize(void)
{
   pid_t pid;

   pid = fork();
   if (pid < 0)
   {
      fprintf(stderr, "Error: failed to fork\n");
      exit(1);
   }
   if (pid > 0)
      exit(0);  // parent exits

   setsid();

   int fd = open("/dev/null", O_RDWR, 0);
   if (fd != -1)
   {
      dup2(fd, STDIN_FILENO);
      dup2(fd, STDOUT_FILENO);
      dup2(fd, STDERR_FILENO);
      if (fd > 2)
         close(fd);
   }
}

int main(int argc, char **argv)
{
   for (int i = 1; i < argc; i++)
   {
      if (strcmp(argv[i], "-i") == 0)
         interactive_mode = true;
   }

   MainServer();
   return 0;
}

#endif

#ifdef BLAK_PLATFORM_LINUX
Bool InMainLoop(void)
{
   return in_main_loop;
}

// On Windows, MainServer() is in main_windows.c
// On Linux, it's here with RunMainLoop() instead of ServiceTimers()
void MainServer(void)
{
   InitInterfaceLocks();

   if (!interactive_mode)
      Daemonize();

   InitMemory();
   InitConfig();
   LoadConfig();

   InitDebug();
   InitChannelBuffer();
   OpenDefaultChannels();

   if (ConfigBool(MYSQL_ENABLED))
   {
      lprintf("Starting PostgreSQL writer\n");
      postgres_connection_options database_options = {
         ConfigStr(MYSQL_HOST),
         ConfigInt(MYSQL_CPORT),
         ConfigStr(MYSQL_USERNAME),
         ConfigStr(MYSQL_PASSWORD),
         ConfigStr(MYSQL_DB),
         ConfigStr(POSTGRES_SSLMODE),
         ConfigStr(POSTGRES_SSLROOTCERT),
         ConfigStr(POSTGRES_SSLCERT),
         ConfigStr(POSTGRES_SSLKEY)
      };
      PostgreSQLInit(&database_options);
   }

   lprintf("Starting %s\n", BlakServLongVersionString());

   InitClass();
   InitMessage();
   InitObject();
   InitList();
   InitTimer();
   InitSession();
   InitResource();
   AStarInit();
   InitRooms();
   InitString();
   InitUser();
   InitAccount();
   InitNameID();
   InitDLlist();
   InitSysTimer();
   InitMotd();
   InitLoadBof();
   InitTime();
   InitGameLock();
   InitBkodInterpret();
   InitBufferPool();
   InitTables();
   AddBuiltInDLlist();
   LoadMotd();
   LoadBof();
   LoadRsc();
   LoadKodbase();
   LoadAdminConstants();
   PauseTimers();

   if (LoadAll() == True)
   {
      SendTopLevelBlakodMessage(GetSystemObjectID(), LOADED_GAME_MSG, 0, NULL);
      DoneLoadAccounts();
   }

   InitCommCli();
   InitParseClient();
   InitProfiling();
   InitAsyncConnections();
   UpdateSecurityRedbook();
   UnpauseTimers();

   StartupComplete();

   if (interactive_mode)
   {
      InitInterface();
      printf("%s\n", BlakServLongVersionString());
      printf("Status: %i accounts, %i timers\n", GetNextAccountID(), GetNumActiveTimers());
   }

   InterfaceUpdate();

   lprintf("Status: %i accounts\n", GetNextAccountID());
   lprintf("-----------------------------------------------------------------------------------------------\n");
   dprintf("-----------------------------------------------------------------------------------------------\n");
   eprintf("-----------------------------------------------------------------------------------------------\n");
   gprintf("-----------------------------------------------------------------------------------------------\n");

   in_main_loop = true;

   AsyncSocketStart();

   if (interactive_mode)
      printf("Server ready. Type 'quit' to shut down.\n");

#ifdef BLAK_PLATFORM_WINDOWS
   SetWindowText(hwndMain, ConfigStr(CONSOLE_CAPTION));
   ServiceTimers();
#else
   RunMainLoop();
#endif

   MainExitServer();
}

void MainExitServer(void)
{
   lprintf("ExitServer terminating server\n");

   ExitAsyncConnections();
   MySQLEnd();
   CloseAllSessions();
   CloseDefaultChannels();

   ResetLoadMotd();
   ResetLoadBof();
   ResetTables();
   ResetBufferPool();
   ResetSysTimer();
   ResetDLlist();
   ResetNameID();
   ResetAccount();
   ResetUser();
   ResetString();
   ExitRooms();
   AStarShutdown();
   ResetResource();
   ResetTimer();
   ResetList();
   ResetObject();
   ResetMessage();
   ResetClass();
   ResetConfig();
   DeleteAllBlocks();
}
#endif // BLAK_PLATFORM_LINUX

void MainReloadGameData(void)
{
   ResetAdminConstants();
   ResetUser();
   ResetString();
   ResetRooms();
   ResetLoadMotd();
   ResetLoadBof();
   ResetDLlist();
   ResetNameID();
   ResetResource();
   ResetTimer();
   ResetList();
   ResetTables();
   ResetObject();
   ResetMessage();
   ResetClass();

   InitNameID();
   LoadMotd();
   LoadBof();
   LoadRsc();
   LoadKodbase();

   UpdateSecurityRedbook();
   LoadAdminConstants();
}

#ifdef BLAK_PLATFORM_WINDOWS
char * GetLastErrorStr()
{
   char *error_str;

   error_str = "No error string";

   FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
      NULL,GetLastError(),MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_US),
      (LPTSTR) &error_str,0,NULL);
   return error_str;
}
#endif
