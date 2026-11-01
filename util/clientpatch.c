// Meridian 59, Copyright 1994-2012 Andrew Kirmse and Chris Kirmse.
// All rights reserved.
//
// This software is distributed under a license that is described in
// the LICENSE file that accompanies it.
//
// Meridian is a registered trademark.
/*
 * clientpatch.c:  Generates a patch list file for updating the client.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <windows.h>
#include <vector>
#include <tuple>
#include <iostream>
#include <clientpatch.h>

#define CPH_NUM_ARGUMENTS 3

typedef std::vector <std::tuple<std::string, std::string, std::string>> FileList;
size_t base_path_len;
typedef std::vector <std::string> ExtList;
static std::string patchinfo_path;
static std::string client_path;

ExtList extensions { ".bgf", ".ogg", ".roo", ".dll", ".rsb", ".exe", ".bsf",
                     ".ttf", ".zip", ".font", ".md", ".png", ".material",
                     ".hlsl", ".dds", ".mesh", ".xml", ".pu", ".compositor",
                     ".imageset", ".layout", ".looknfeel", ".scheme", ".avi", ".otf" };

bool FindMatchingFiles(std::string path, FileList *files);
/***************************************************************************/
void Usage(void)
{
   printf("Usage: clientpatch C:\\path\\to\\patchinfo.txt C:\\path\\to\\client\n");
   exit(1);
}
/***************************************************************************/
double GetMicroCountDouble()
{
   static LARGE_INTEGER microFrequency;
   LARGE_INTEGER now;

   if (microFrequency.QuadPart == 0)
      QueryPerformanceFrequency(&microFrequency);

   if (microFrequency.QuadPart == 0)
   {
      printf("GetMicroCount can't get frequency\n");
      return 0;
   }

   QueryPerformanceCounter(&now);
   return ((double)now.QuadPart * 1000.0) / (double)microFrequency.QuadPart;
}
/***************************************************************************/
int main(int argc, char **argv)
{
   FileList files; // Data structure for fullpath, basepath, filename.
   json_t *FileArray; // Array of JSON objects.
   json_t *ExecArray; // Separate array for executables.
   char *strptr;
   // Init the arrays.
   FileArray = json_array();
   ExecArray = json_array();

   if (argc != CPH_NUM_ARGUMENTS)
      Usage();

   patchinfo_path = argv[1]; // File we will write to, including full path.
   client_path = argv[2]; // Path to the client we will scan.

   // Allow using input paths with and without the trailing '\'
   if (client_path.back() != '\\')
      client_path.append("\\");

   // Record start time.
   double startTime = GetMicroCountDouble();

   // Get the path length, used to create relative path later.
   base_path_len = client_path.length() - 1;
   printf("Scanning, please wait...\n");

   if (FindMatchingFiles(client_path, &files))
   {
      // Iterate through our found files, create JSON objects and add to array.
      for (FileList::iterator it = files.begin(); it != files.end(); ++it)
      {
         // Add executables to a separate array, because we want them
         // at the end of the JSON array printed to file. Means that
         // during the update process, the last thing updated are
         // client executables.
         strptr = strrchr((char *)std::get<2>(*it).c_str(), '.');
         if (!strptr)
            continue;
         if (stricmp(strptr, ".exe") == 0)
            json_array_append(ExecArray, GenerateCacheFile(std::get<0>(*it).c_str(),
               std::get<1>(*it).c_str(), std::get<2>(*it).c_str()));
         else
            json_array_append(FileArray, GenerateCacheFile(std::get<0>(*it).c_str(),
               std::get<1>(*it).c_str(), std::get<2>(*it).c_str()));
      }
      // Join the arrays
      if (json_array_size(ExecArray) > 0)
         json_array_extend(FileArray, ExecArray);

      // Print the file to the give patchinfo.txt.
      json_dump_file(FileArray, patchinfo_path.c_str(), JSON_INDENT(1));
         printf("Successfully added %zu files.\n", files.size());
   }

   printf("Scanning completed in %.3f ms.\n", GetMicroCountDouble() - startTime);
}

bool FindMatchingFiles(std::string path, FileList *files)
{
   HANDLE hFindFile;
   WIN32_FIND_DATA search_data;
   std::string search_dir; // Current directory.
   std::string new_dir; // Used when switching directories.
   std::string basepath_dir; // Relative path from client directory.
   char *strptr; // Pointer used for file extension checking.

   // Searching for any file/directory, append wildcard to path.
   search_dir = path;
   search_dir.append("*");

   hFindFile = FindFirstFile(search_dir.c_str(), &search_data);
   if (hFindFile == INVALID_HANDLE_VALUE)
      return false;

   // Create the relative path to file from client directory.
   // Subtract the length of the path-to-client calculated earlier.
   basepath_dir.assign(path);
   basepath_dir.replace(basepath_dir.begin(), basepath_dir.begin() + base_path_len, "");

   do
   {
      if (search_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
      {
         // Skip the "." and ".." directories.
         if (strcmp(search_data.cFileName, ".") != 0
            && strcmp(search_data.cFileName, "..") != 0)
         {
            // Create a new path with backslashes and the next directory
            // to search. Recurse on it.
            new_dir = path;
            new_dir.append(search_data.cFileName);
            new_dir.append("\\");
            FindMatchingFiles(new_dir, files);
         }
      }
      else
      {
         // Get the extension, including '.' and check against extension list.
         strptr = strrchr(search_data.cFileName, '.');
         if (!strptr)
            continue;
         for (ExtList::iterator it = extensions.begin(); it != extensions.end(); ++it)
         {
            // If we match an extension, add to the FileList structure.
            // TODO: This could probably be faster.
            if (stricmp(strptr, it->c_str()) == 0)
            {
               files->push_back(std::make_tuple(path, basepath_dir, search_data.cFileName));
               break;
            }
         }
      }
   } while (FindNextFile(hFindFile, &search_data));

   FindClose(hFindFile);

   return true;
}
