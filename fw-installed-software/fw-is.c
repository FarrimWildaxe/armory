#include <windows.h>
#include <winreg.h>
#include "beacon.h"

#define MAX_KEY_LENGTH 255
#define MAX_VALUE_NAME 255 // Max value is 16383, but here 256 is used due to stack constrains


DECLSPEC_IMPORT WINADVAPI LONG WINAPI ADVAPI32$RegOpenKeyExA (HKEY, LPCSTR, DWORD, REGSAM, PHKEY);
DECLSPEC_IMPORT WINADVAPI LONG WINAPI ADVAPI32$RegCloseKey (HKEY);
DECLSPEC_IMPORT WINADVAPI LONG WINAPI ADVAPI32$RegQueryInfoKeyA (HKEY, LPSTR, LPDWORD, LPDWORD, LPDWORD, LPDWORD, LPDWORD, LPDWORD, LPDWORD, LPDWORD, LPDWORD, PFILETIME);
DECLSPEC_IMPORT WINADVAPI LONG WINAPI ADVAPI32$RegEnumKeyExA (HKEY, DWORD, LPSTR, LPDWORD, LPDWORD, LPSTR, LPDWORD, PFILETIME);
DECLSPEC_IMPORT WINADVAPI LONG WINAPI ADVAPI32$RegQueryValueExA (HKEY, LPCSTR, LPDWORD, LPDWORD, LPBYTE, LPDWORD);


void query_subkey(HKEY parentKey, const char* subKeyName) {
    HKEY hKey;
   
    CHAR    achClass[MAX_PATH] = "";  // buffer for class name 
    DWORD    cchClassName = MAX_PATH;  // size of class string 
    DWORD    cSubKeys=0;               // number of subkeys 
    DWORD    cbMaxSubKey;              // longest subkey size 
    DWORD    cchMaxClass;              // longest class string 
    DWORD    cValues;              // number of values for key 
    DWORD    cchMaxValue;          // longest value name 
    DWORD    cbMaxValueData;       // longest value data 
    DWORD    cbSecurityDescriptor; // size of security descriptor 
    FILETIME ftLastWriteTime;      // last write time 

    LONG lRes = ADVAPI32$RegOpenKeyExA(parentKey, subKeyName, 0, KEY_READ, &hKey);
    if (lRes == ERROR_SUCCESS) {

        DWORD retCode = ADVAPI32$RegQueryInfoKeyA(hKey, achClass, &cchClassName, NULL, &cSubKeys, &cbMaxSubKey,
            &cchMaxClass, &cValues, &cchMaxValue, &cbMaxValueData, &cbSecurityDescriptor, &ftLastWriteTime);

        if (retCode == ERROR_SUCCESS) {
            if  (cValues >= 3) {
                enum { BUF_SIZE = 512 };
                CHAR szBuffer[BUF_SIZE] = { 0 };
                DWORD dwBufferSize = BUF_SIZE;

                enum { KEY_NAMES_COUNT = 3 };
                const LPCSTR KEY_NAMES[KEY_NAMES_COUNT] = {
                    "DisplayName",
                    "DisplayVersion",
                    "Publisher"
                };
 
                for (int v = 0; v < KEY_NAMES_COUNT; ++v) {
                    szBuffer[0] = 0;

                    ULONG nError = ADVAPI32$RegQueryValueExA(hKey, KEY_NAMES[v], 0, NULL, (LPBYTE)szBuffer, &dwBufferSize);
                    if (nError == ERROR_SUCCESS) {
                        BeaconPrintf(CALLBACK_OUTPUT, "%s\t", szBuffer);
                    }
                    else if (nError != ERROR_FILE_NOT_FOUND && nError != ERROR_MORE_DATA) {
                        BeaconPrintf(CALLBACK_ERROR, "[!] Error code: %d. Failed to query value: '%s'.\n", nError, KEY_NAMES[v]);
                    }
                }
                BeaconPrintf(CALLBACK_OUTPUT, "\n");
            }
        }
        else {
            BeaconPrintf(CALLBACK_ERROR, "[!] Error code: %d. Failed to query info key: '%s'.\n", retCode, subKeyName);
        }                

        lRes = ADVAPI32$RegCloseKey(hKey);
        if (lRes != ERROR_SUCCESS) {
            BeaconPrintf(CALLBACK_ERROR, "[!] Error code: %d. Failed to close registry key: '%s'.\n", lRes, subKeyName);
        }
    }
    else {
        BeaconPrintf(CALLBACK_ERROR, "[!] Error code: %d. Failed to open registry key: '%s'.\n", lRes, subKeyName);
    }
}

void query_key(HKEY hKey) { 
    CHAR    achKey[MAX_KEY_LENGTH];   // buffer for subkey name
    DWORD    cbName;                   // size of name string 
    CHAR    achClass[MAX_PATH] = "";  // buffer for class name 
    DWORD    cchClassName = MAX_PATH;  // size of class string 
    DWORD    cSubKeys=0;               // number of subkeys 
    DWORD    cbMaxSubKey;              // longest subkey size 
    DWORD    cchMaxClass;              // longest class string 
    DWORD    cValues;              // number of values for key 
    DWORD    cchMaxValue;          // longest value name 
    DWORD    cbMaxValueData;       // longest value data 
    DWORD    cbSecurityDescriptor; // size of security descriptor 
    FILETIME ftLastWriteTime;      // last write time 
    
    DWORD retCode = ADVAPI32$RegQueryInfoKeyA(hKey, achClass, &cchClassName, NULL, &cSubKeys, &cbMaxSubKey,
        &cchMaxClass, &cValues, &cchMaxValue, &cbMaxValueData, &cbSecurityDescriptor, &ftLastWriteTime);

    if (cSubKeys > 0) {

        for (DWORD i = 0; i < cSubKeys; i++) { 
            cbName = MAX_KEY_LENGTH;
            retCode = ADVAPI32$RegEnumKeyExA (hKey, i, achKey, &cbName, NULL, NULL, NULL, &ftLastWriteTime); 

            if (retCode == ERROR_SUCCESS) {
                query_subkey(hKey, achKey);
            }
            else {
                BeaconPrintf(CALLBACK_ERROR, "[!] Error code: %d. Failed to enum key: '%s'.\n", achKey, retCode);
                break;
            }
        }
    }
}

void go(char /* *args */, int /* len */) {
    enum { PATHS_COUNT = 2 };
    const LPCSTR APP_REG_PATHS[PATHS_COUNT] = {
      "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall",
      "SOFTWARE\\WOW6432NODE\\Microsoft\\Windows\\CurrentVersion\\Uninstall",
    };

    enum { HKEYS_COUNT = 2 };
    const HKEY HKEYS[HKEYS_COUNT] = {
        HKEY_LOCAL_MACHINE,
        HKEY_CURRENT_USER
    };

    HKEY hKey;
    LONG lRes = ERROR_FILE_NOT_FOUND;

    BeaconPrintf(CALLBACK_OUTPUT, "[+] started.\n");

    BeaconPrintf(CALLBACK_OUTPUT, "Installed software:\n");
    BeaconPrintf(CALLBACK_OUTPUT, "=========================================================\n");
    BeaconPrintf(CALLBACK_OUTPUT, "DisplayName\tDisplayVersion\tPublisher\n");
    BeaconPrintf(CALLBACK_OUTPUT, "=========================================================\n");

    for (int hkey_idx = 0; hkey_idx < HKEYS_COUNT; ++hkey_idx) {
        for (int path_idx = 0; path_idx < PATHS_COUNT; ++path_idx) {

            lRes = ADVAPI32$RegOpenKeyExA(HKEYS[hkey_idx], APP_REG_PATHS[path_idx], 0, KEY_READ, &hKey);
            if (lRes == ERROR_SUCCESS) {
                
                query_key(hKey);

                lRes = ADVAPI32$RegCloseKey(hKey);
                if (lRes != ERROR_SUCCESS) {
                    BeaconPrintf(CALLBACK_ERROR, "[!] Error code: %d. Failed to close registry key: '%s' (hkey_idx: %d).\n",
                        lRes, APP_REG_PATHS[path_idx], hkey_idx);
                    break;
                }
            }
            else if (lRes != ERROR_FILE_NOT_FOUND) {
                BeaconPrintf(CALLBACK_ERROR, "[!] Error code: %d. Failed to open registry key: '%s' (hkey_idx: %d).\n",
                    lRes, APP_REG_PATHS[path_idx], hkey_idx);
                break;
            }
        }
    }

    BeaconPrintf(CALLBACK_OUTPUT, "[+] ended.\n");
}