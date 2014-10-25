// Meridian 59, Copyright 1994-2012 Andrew Kirmse and Chris Kirmse.
// All rights reserved.
//
// This software is distributed under a license that is described in
// the LICENSE file that accompanies it.
//
// Meridian is a registered trademark.
/*
 * stats.c:  DLL that allows user to modify their stats.
 *   This DLL is invoked by the server when needed.
 */

#include "client.h"
#include "stats.h"

HINSTANCE hInst;            // Handle of this DLL

ClientInfo *cinfo;         // Holds data passed from main client
Bool        exiting;

static Bool HandleStatChangeRequest(char *ptr, long len);

// Server message handler table
static handler_struct handler_table[] = {
{ BP_STAT_CHANGE,        HandleStatChangeRequest },
{ 0, NULL},
};

// TODO: rework these messages
// Client message table
client_message msg_table[] = {
{ BP_CHANGED_STATS,      { PARAM_BYTE,  PARAM_BYTE, PARAM_BYTE, PARAM_BYTE, PARAM_BYTE, PARAM_BYTE, PARAM_END }, },
{ 0,                       { PARAM_END }, },
};

/****************************************************************************/
BOOL WINAPI DllMain(HINSTANCE hModule, DWORD reason, LPVOID reserved)
{
   switch (reason)
   {
   case DLL_PROCESS_ATTACH:
      hInst = hModule;
      break;

   case DLL_PROCESS_DETACH:
//      AbortCharDialogs();
      break;
   }
   return TRUE;
}
/****************************************************************************/
void WINAPI GetModuleInfo(ModuleInfo *info, ClientInfo *client_info)
{
   info->event_mask = EVENT_SERVERMSG | EVENT_STATECHANGED;
   info->priority   = PRIORITY_NORMAL;
   info->module_id  = MODULE_ID;
   cinfo = client_info;    // Save client info for our use later

   exiting = False;
   // Ask server for characters to pick from
   //RequestCharacters();
}

/* event handlers */
/****************************************************************************/
/*
 * EVENT_SERVERMSG
 */
/****************************************************************************/
Bool WINAPI EventServerMessage(char *message, long len)
{
   Bool retval;

   retval = LookupMessage(message, len, handler_table);

   // If we handle message, don't pass it on to anyone else
   if (retval == True)
     return False;

   return True;    // Allow other modules to get other messages
}
/********************************************************************/
Bool HandleStatChangeRequest(char *ptr, long len)
{
    BYTE might, intellect, stamina, agility, mysticism, aim = 0;
	
	Extract(&ptr, &might, 1);	
	Extract(&ptr, &intellect, 1);
	Extract(&ptr, &stamina, 1);
	Extract(&ptr, &agility, 1);
	Extract(&ptr, &mysticism, 1);
	Extract(&ptr, &aim, 1);
	
	// TODO: data type mismatch
	int myStats[] = {might, intellect, stamina, agility, mysticism, aim};

    debug(("HandleStatChangeRequest() call. Server sent might=%i, int=%i, \
			stam=%i, agi=%i, myst=%i, aim=%i\n", might, intellect, stamina,
			agility, mysticism, aim));
	MakeChar(myStats);
	
    return true;
}
/****************************************************************************/
Bool WINAPI EventStateChanged(int old_state, int new_state)
{
   if (old_state == GAME_PICKCHAR && new_state != GAME_PICKCHAR)
   {
      // Force redraw of main window, to get rid of dialogs
      UpdateWindow(cinfo->hMain);
      //AbortCharDialogs();
   }
   return True;
}
