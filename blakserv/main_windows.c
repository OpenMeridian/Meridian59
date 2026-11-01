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

void MainServer()
{
   HANDLE hProcess = GetCurrentProcess();
   SetPriorityClass(hProcess, HIGH_PRIORITY_CLASS);
#if defined(DISABLE_HT)
   // don't allow to run on HT cores
   // 8cores + 8ht --> 0x5555 = 101010101010101
   SetProcessAffinityMask(hProcess, 0x5555);
#endif

	InitInterfaceLocks(); 
	
	InitInterface(); /* starts a thread with the window */

	InitMemory(); /* memory needs channels in general, but need to start before config,
	so just be careful. */
	
	InitConfig();
	LoadConfig();		/* must be nearly first since channels use it */

	InitDebug();
	
	InitChannelBuffer();
	
	OpenDefaultChannels();

	if (ConfigBool(MYSQL_ENABLED))
	{
	#ifdef BLAK_USE_POSTGRESQL
		lprintf("Starting PostgreSQL writer");
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
	#else
		lprintf("Starting MySQL writer");
		MySQLInit(ConfigStr(MYSQL_HOST), ConfigInt(MYSQL_CPORT), ConfigStr(MYSQL_USERNAME),
			ConfigStr(MYSQL_PASSWORD), ConfigStr(MYSQL_DB));
	#endif
	}

	lprintf("Starting %s\n",BlakServLongVersionString());

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
	/* this loaded_game_msg tells it to disconnect all blakod info about sessions,
		* that were logged on when we saved */
		
		SendTopLevelBlakodMessage(GetSystemObjectID(),LOADED_GAME_MSG,0,NULL);
		DoneLoadAccounts();
	}
	
	/* these must be after LoadAll and ClearList */
	InitCommCli(); 
	InitParseClient(); 
	InitProfiling();
	InitAsyncConnections();
	UpdateSecurityRedbook();
	UnpauseTimers();

	ServiceTimers(); /* returns if server termiated */

	MainExitServer();
}

void MainExitServer()
{
	lprintf("ExitServer terminating server\n");
	
	ExitAsyncConnections();
	
	CloseAllSessions(); /* gotta do this before anything, cause it uses kod, accounts */
	
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
	// ExitRooms calls ResetRooms in addition to clearing the array memory.
	ExitRooms();
	AStarShutdown();
	ResetResource();
	ResetTimer();
	ResetList();
	ResetObject();
	ResetMessage();
	ResetClass();

	if (ConfigBool(MYSQL_ENABLED))
		MySQLEnd();
	
	ResetConfig();
	
	DeleteAllBlocks();
}
