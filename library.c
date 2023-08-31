/**
 * Copyright Jerily LTD. All Rights Reserved.
 * SPDX-FileCopyrightText: 2023 Neofytos Dimitriou (neo@jerily.cy)
 * SPDX-License-Identifier: MIT.
 */
#include "library.h"
#include "cJSON.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

#define XSTR(s) STR(s)
#define STR(s) #s

#ifdef DEBUG
# define DBG(x) x
#else
# define DBG(x)
#endif

#define CheckArgs(min,max,n,msg) \
                 if ((objc < min) || (objc >max)) { \
                     Tcl_WrongNumArgs(interp, n, objv, msg); \
                     return TCL_ERROR; \
                 }

#define CMD_NAME(s, internal) sprintf((s), "_TJSON_%p", (internal))
static int tjson_ModuleInitialized;

static Tcl_HashTable tjson_NodeToInternal_HT;
static Tcl_Mutex tjson_NodeToInternal_HT_Mutex;

static int
tjson_RegisterNode(const char *name, cJSON *internal) {

    Tcl_HashEntry *entryPtr;
    int newEntry;
    Tcl_MutexLock(&tjson_NodeToInternal_HT_Mutex);
    entryPtr = Tcl_CreateHashEntry(&tjson_NodeToInternal_HT, (char *) name, &newEntry);
    if (newEntry) {
        Tcl_SetHashValue(entryPtr, (ClientData) internal);
    }
    Tcl_MutexUnlock(&tjson_NodeToInternal_HT_Mutex);

    DBG(fprintf(stderr, "--> RegisterNode: name=%s internal=%p %s\n", name, internal,
                newEntry ? "entered into" : "already in"));

    return newEntry;
}

static int
tjson_UnregisterNode(const char *name) {

    Tcl_HashEntry *entryPtr;

    Tcl_MutexLock(&tjson_NodeToInternal_HT_Mutex);
    entryPtr = Tcl_FindHashEntry(&tjson_NodeToInternal_HT, (char *) name);
    if (entryPtr != NULL) {
        Tcl_DeleteHashEntry(entryPtr);
    }
    Tcl_MutexUnlock(&tjson_NodeToInternal_HT_Mutex);

    DBG(fprintf(stderr, "--> UnregisterNode: name=%s entryPtr=%p\n", name, entryPtr));

    return entryPtr != NULL;
}

static cJSON *
tjson_GetInternalFromNode(const char *name) {
    cJSON *internal = NULL;
    Tcl_HashEntry *entryPtr;

    Tcl_MutexLock(&tjson_NodeToInternal_HT_Mutex);
    entryPtr = Tcl_FindHashEntry(&tjson_NodeToInternal_HT, (char *) name);
    if (entryPtr != NULL) {
        internal = (cJSON *) Tcl_GetHashValue(entryPtr);
    }
    Tcl_MutexUnlock(&tjson_NodeToInternal_HT_Mutex);

    return internal;
}



static Tcl_Obj *tjson_TreeToSimple(Tcl_Interp *interp, cJSON *item) {

    switch ((item->type) & 0xFF)
    {
        case cJSON_NULL:
            return Tcl_NewStringObj("", 0);
        case cJSON_False:
            return Tcl_NewBooleanObj(0);
        case cJSON_True:
            return Tcl_NewBooleanObj(1);
        case cJSON_Number:
            double d = item->valuedouble;
            if (isnan(d) || isinf(d)) {
                return Tcl_NewStringObj("", 0);
            } else if(d == (double)item->valueint) {
                return Tcl_NewIntObj(item->valueint);
            } else {
                return Tcl_NewDoubleObj(item->valuedouble);
            }
        case cJSON_Raw:
        {
            if (item->valuestring == NULL)
            {
                return Tcl_NewStringObj("", 0);
            }

            return Tcl_NewStringObj(item->valuestring, -1);
        }

        case cJSON_String:
            return Tcl_NewStringObj(item->valuestring, -1);
        case cJSON_Array:
            Tcl_Obj *listPtr = Tcl_NewListObj(0, NULL);
            cJSON *current_element = item->child;
            while (current_element != NULL) {
                Tcl_ListObjAppendElement(interp, listPtr, tjson_TreeToSimple(interp, current_element));
                current_element = current_element->next;
            }
            return listPtr;
        case cJSON_Object:
            Tcl_Obj *dictPtr = Tcl_NewDictObj();
            cJSON *current_item = item->child;
            while (current_item) {
                Tcl_DictObjPut(
                        interp,
                        dictPtr,
                        Tcl_NewStringObj(current_item->string, -1),
                        tjson_TreeToSimple(interp, current_item));
                current_item = current_item->next;
            }
            return dictPtr;
        default:
            return NULL;
    }
}

static Tcl_Obj *tjson_TreeToTyped(Tcl_Interp *interp, cJSON *item) {

    Tcl_Obj *resultPtr = Tcl_NewListObj(0, NULL);
    switch ((item->type) & 0xFF)
    {
        case cJSON_NULL:
            Tcl_ListObjAppendElement(interp, resultPtr, Tcl_NewStringObj("S", 1));
            Tcl_ListObjAppendElement(interp, resultPtr, Tcl_NewStringObj("", 0));
            return resultPtr;
        case cJSON_False:
            Tcl_ListObjAppendElement(interp, resultPtr, Tcl_NewStringObj("BOOL", 4));
            Tcl_ListObjAppendElement(interp, resultPtr, Tcl_NewBooleanObj(0));
            return resultPtr;
        case cJSON_True:
            Tcl_ListObjAppendElement(interp, resultPtr, Tcl_NewStringObj("BOOL", 4));
            Tcl_ListObjAppendElement(interp, resultPtr, Tcl_NewBooleanObj(1));
            return resultPtr;
        case cJSON_Number:
            double d = item->valuedouble;
            if (isnan(d) || isinf(d)) {
                Tcl_ListObjAppendElement(interp, resultPtr, Tcl_NewStringObj("S", 1));
                Tcl_ListObjAppendElement(interp, resultPtr, Tcl_NewStringObj("", 0));
                return resultPtr;
            } else if(d == (double)item->valueint) {
                Tcl_ListObjAppendElement(interp, resultPtr, Tcl_NewStringObj("N", 1));
                Tcl_ListObjAppendElement(interp, resultPtr, Tcl_NewIntObj(item->valueint));
                return resultPtr;
            } else {
                Tcl_ListObjAppendElement(interp, resultPtr, Tcl_NewStringObj("N", 1));
                Tcl_ListObjAppendElement(interp, resultPtr, Tcl_NewDoubleObj(item->valuedouble));
                return resultPtr;
            }
        case cJSON_Raw:
        {
            if (item->valuestring == NULL)
            {
                Tcl_ListObjAppendElement(interp, resultPtr, Tcl_NewStringObj("S", 1));
                Tcl_ListObjAppendElement(interp, resultPtr, Tcl_NewStringObj("", 0));
                return resultPtr;
            }

            Tcl_ListObjAppendElement(interp, resultPtr, Tcl_NewStringObj("S", 1));
            Tcl_ListObjAppendElement(interp, resultPtr, Tcl_NewStringObj(item->valuestring, -1));
        }

        case cJSON_String:
            Tcl_ListObjAppendElement(interp, resultPtr, Tcl_NewStringObj("S", 1));
            Tcl_ListObjAppendElement(interp, resultPtr, Tcl_NewStringObj(item->valuestring, -1));
            return resultPtr;
        case cJSON_Array:
            Tcl_Obj *listPtr = Tcl_NewListObj(0, NULL);
            cJSON *current_element = item->child;
            while (current_element != NULL) {
                Tcl_ListObjAppendElement(interp, listPtr, tjson_TreeToTyped(interp, current_element));
                current_element = current_element->next;
            }
            Tcl_ListObjAppendElement(interp, resultPtr, Tcl_NewStringObj("L", 1));
            Tcl_ListObjAppendElement(interp, resultPtr, listPtr);
            return resultPtr;
        case cJSON_Object:
            Tcl_Obj *dictPtr = Tcl_NewDictObj();
            cJSON *current_item = item->child;
            while (current_item) {
                Tcl_DictObjPut(
                        interp,
                        dictPtr,
                        Tcl_NewStringObj(current_item->string, -1),
                        tjson_TreeToTyped(interp, current_item));
                current_item = current_item->next;
            }
            Tcl_ListObjAppendElement(interp, resultPtr, Tcl_NewStringObj("M", 1));
            Tcl_ListObjAppendElement(interp, resultPtr, dictPtr);
            return resultPtr;
        default:
            return resultPtr;
    }
}

static int tjson_JsonToTypedCmd(ClientData  clientData, Tcl_Interp *interp, int objc, Tcl_Obj * const objv[] ) {
    DBG(fprintf(stderr, "ParseCmd\n"));
    CheckArgs(2,2,1,"json");

    int length;
    const char *json = Tcl_GetStringFromObj(objv[1], &length);

    cJSON *root_structure = cJSON_ParseWithLength(json, length);
    Tcl_Obj *resultPtr = tjson_TreeToTyped(interp, root_structure);
    cJSON_Delete(root_structure);

    Tcl_SetObjResult(interp, resultPtr);
    return TCL_OK;
}

static int tjson_JsonToSimpleCmd(ClientData  clientData, Tcl_Interp *interp, int objc, Tcl_Obj * const objv[] ) {
    DBG(fprintf(stderr, "ParseCmd\n"));
    CheckArgs(2,2,1,"json");

    int length;
    const char *json = Tcl_GetStringFromObj(objv[1], &length);

    int simple = 0;
    if (objc == 3) {
        Tcl_GetBoolean(interp, Tcl_GetString(objv[2]), &simple);
    }

    cJSON *root_structure = cJSON_ParseWithLength(json, length);
    Tcl_Obj *resultPtr = tjson_TreeToSimple(interp, root_structure);
    cJSON_Delete(root_structure);

    Tcl_SetObjResult(interp, resultPtr);
    return TCL_OK;
}

static int tjson_ParseCmd(ClientData  clientData, Tcl_Interp *interp, int objc, Tcl_Obj * const objv[] ) {
    DBG(fprintf(stderr, "ParseCmd\n"));
    CheckArgs(2,2,1,"json");

    int length;
    const char *json = Tcl_GetStringFromObj(objv[1], &length);

    cJSON *root_structure = cJSON_ParseWithLength(json, length);

    char handle[80];
    CMD_NAME(handle, root_structure);
    tjson_RegisterNode(handle, root_structure);
//    cJSON_Delete(root_structure);
    Tcl_SetObjResult(interp, Tcl_NewStringObj(handle, -1));
    return TCL_OK;
}

static int tjson_DestroyCmd(ClientData  clientData, Tcl_Interp *interp, int objc, Tcl_Obj * const objv[] ) {
    DBG(fprintf(stderr, "DestroyCmd\n"));
    CheckArgs(2,2,1,"handle");

    const char *handle = Tcl_GetString(objv[1]);
    cJSON *root_structure = tjson_GetInternalFromNode(handle);
    tjson_UnregisterNode(handle);
    cJSON_Delete(root_structure);

    return TCL_OK;
}

static int tjson_SizeCmd(ClientData  clientData, Tcl_Interp *interp, int objc, Tcl_Obj * const objv[] ) {
    DBG(fprintf(stderr, "SizeCmd\n"));
    CheckArgs(2,2,1,"handle");

    const char *handle = Tcl_GetString(objv[1]);
    cJSON *root_structure = tjson_GetInternalFromNode(handle);
    int size = cJSON_GetArraySize(root_structure);
    Tcl_SetObjResult(interp, Tcl_NewIntObj(size));
    return TCL_OK;
}

static int tjson_CreateItemFromSpec(Tcl_Interp *interp, Tcl_Obj *specPtr, cJSON **item) {
    // "specPtr" is a list of two elements: type and value
    int length;
    Tcl_ListObjLength(interp, specPtr, &length);
    if (length != 2) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid spec length", -1));
        return TCL_ERROR;
    }
    Tcl_Obj *typePtr, *valuePtr;
    Tcl_ListObjIndex(interp, specPtr, 0, &typePtr);
    Tcl_ListObjIndex(interp, specPtr, 1, &valuePtr);

    int typeLength;
    const char *type = Tcl_GetStringFromObj(typePtr, &typeLength);
    switch (type[0]) {
        case 'S':
            *item = cJSON_CreateString(Tcl_GetString(valuePtr));
            return TCL_OK;
        case 'N':
            double value_double;
            Tcl_GetDoubleFromObj(interp, valuePtr, &value_double);
            *item = cJSON_CreateNumber(value_double);
            return TCL_OK;
        case 'B':
            if (typeLength == 4 && 0 == strcmp("BOOL", type)) {
                int flag;
                Tcl_GetBooleanFromObj(NULL, specPtr, &flag);
                *item = cJSON_CreateBool(flag);
                return TCL_OK;
            } else {
                *item = cJSON_CreateNull();
                return TCL_OK;
            }
        case 'M':
            cJSON *obj = cJSON_CreateObject();
            // iterate "valuePtr" as a dict and add each item to the object "obj"
            Tcl_DictSearch search;
            Tcl_Obj *key, *elemSpecPtr;
            int done;
            if (Tcl_DictObjFirst(interp, valuePtr, &search,
                                 &key, &elemSpecPtr, &done) != TCL_OK) {
                return TCL_ERROR;
            }
            for (; !done; Tcl_DictObjNext(&search, &key, &elemSpecPtr, &done)) {
                cJSON *elem = NULL;
                if (TCL_OK != tjson_CreateItemFromSpec(interp, elemSpecPtr, &elem)) {
                    return TCL_ERROR;
                }
                cJSON_AddItemToObject(obj, Tcl_GetString(key), elem);
            }
            *item = obj;
            return TCL_OK;
        case 'L':
            cJSON *arr = cJSON_CreateArray();
            // iterate "valuePtr" as a list and add each item to the object "arr"
            int listLength;
            Tcl_ListObjLength(interp, valuePtr, &listLength);
            for (int i = 0; i < listLength; i++) {
                Tcl_Obj *elemSpecPtr;
                Tcl_ListObjIndex(interp, valuePtr, i, &elemSpecPtr);
                cJSON *elem = NULL;
                if (TCL_OK != tjson_CreateItemFromSpec(interp, elemSpecPtr, &elem)) {
                    return TCL_ERROR;
                }
                cJSON_AddItemToArray(arr, elem);
            }
            *item = arr;
            return TCL_OK;
        default:
            Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid type in spec", -1));
            return TCL_ERROR;
    }
}

static int tjson_AddItemToObjectCmd(ClientData  clientData, Tcl_Interp *interp, int objc, Tcl_Obj * const objv[] ) {
    DBG(fprintf(stderr, "AddItemToObjectCmd\n"));
    CheckArgs(4,4,1,"handle key typed_item_spec");

    const char *handle = Tcl_GetString(objv[1]);
    cJSON *root_structure = tjson_GetInternalFromNode(handle);

    if (!cJSON_IsObject(root_structure)) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("node is not an object", -1));
        return TCL_ERROR;
    }

    cJSON *item = NULL;
    if (TCL_OK != tjson_CreateItemFromSpec(interp, objv[3], &item)) {
        return TCL_ERROR;
    }
    cJSON_AddItemToObject(root_structure, Tcl_GetString(objv[2]), item);
    return TCL_OK;
}

static int tjson_AddItemToArrayCmd(ClientData  clientData, Tcl_Interp *interp, int objc, Tcl_Obj * const objv[] ) {
    DBG(fprintf(stderr, "AddItemToArrayCmd\n"));
    CheckArgs(3,3,1,"handle typed_item_spec");

    const char *handle = Tcl_GetString(objv[1]);
    cJSON *root_structure = tjson_GetInternalFromNode(handle);

    if (!cJSON_IsArray(root_structure)) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("node is not an array", -1));
        return TCL_ERROR;
    }

    cJSON *item = NULL;
    if (TCL_OK != tjson_CreateItemFromSpec(interp, objv[2], &item)) {
        return TCL_ERROR;
    }
    cJSON_AddItemToArray(root_structure, item);

    return TCL_OK;
}

static int tjson_InsertItemInArrayCmd(ClientData  clientData, Tcl_Interp *interp, int objc, Tcl_Obj * const objv[] ) {
    DBG(fprintf(stderr, "InsertItemInArrayCmd\n"));
    CheckArgs(4,4,1,"handle index typed_item_spec");

    const char *handle = Tcl_GetString(objv[1]);
    cJSON *root_structure = tjson_GetInternalFromNode(handle);
    if (!cJSON_IsArray(root_structure)) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("node is not an array", -1));
        return TCL_ERROR;
    }

    int index;
    if (TCL_OK != Tcl_GetIntFromObj(interp, objv[2], &index)) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid index", -1));
        return TCL_ERROR;
    }

    cJSON *item = NULL;
    if (TCL_OK != tjson_CreateItemFromSpec(interp, objv[3], &item)) {
        return TCL_ERROR;
    }
    cJSON_InsertItemInArray(root_structure, index, item);

    return TCL_OK;
}

static int tjson_ToSimpleCmd(ClientData  clientData, Tcl_Interp *interp, int objc, Tcl_Obj * const objv[] ) {
    DBG(fprintf(stderr, "AppendItemToArrayCmd\n"));
    CheckArgs(2,2,1,"handle");

    const char *handle = Tcl_GetString(objv[1]);
    cJSON *root_structure = tjson_GetInternalFromNode(handle);
    Tcl_Obj *resultPtr = tjson_TreeToSimple(interp, root_structure);
    Tcl_SetObjResult(interp, resultPtr);
    return TCL_OK;
}

static int tjson_ToTypedCmd(ClientData  clientData, Tcl_Interp *interp, int objc, Tcl_Obj * const objv[] ) {
    DBG(fprintf(stderr, "AppendItemToArrayCmd\n"));
    CheckArgs(2,2,1,"handle");

    const char *handle = Tcl_GetString(objv[1]);
    cJSON *root_structure = tjson_GetInternalFromNode(handle);
    Tcl_Obj *resultPtr = tjson_TreeToTyped(interp, root_structure);
    Tcl_SetObjResult(interp, resultPtr);
    return TCL_OK;
}

const char *tjson_EscapeJsonString(Tcl_Obj *objPtr) {
    int length;
    const char *str = Tcl_GetStringFromObj(objPtr, &length);
    Tcl_Obj *resultPtr = Tcl_NewStringObj("", 0);
    // loop through each character of the input string
    for (int i = 0; i < length; i++) {
        char c = str[i];
        switch (c) {
            case '"':
                Tcl_AppendStringsToObj(resultPtr, "\\\"", NULL);
                break;
            case '\\':
                Tcl_AppendStringsToObj(resultPtr, "\\\\", NULL);
                break;
            case '\b':
                Tcl_AppendStringsToObj(resultPtr, "\\b", NULL);
                break;
            case '\f':
                Tcl_AppendStringsToObj(resultPtr, "\\f", NULL);
                break;
            case '\n':
                Tcl_AppendStringsToObj(resultPtr, "\\n", NULL);
                break;
            case '\r':
                Tcl_AppendStringsToObj(resultPtr, "\\r", NULL);
                break;
            case '\t':
                Tcl_AppendStringsToObj(resultPtr, "\\t", NULL);
                break;
            default:
                if (c < 32) {
                    Tcl_AppendStringsToObj(resultPtr, "\\u00", NULL);
                    char hex[3];
                    sprintf(hex, "%02x", c);
                    Tcl_AppendStringsToObj(resultPtr, hex, NULL);
                } else {
                    char tempstr[2];
                    tempstr[0] = c;
                    tempstr[1] = '\0';
                    Tcl_AppendStringsToObj(resultPtr, tempstr, NULL);
                }
        }
    }
    return Tcl_GetString(resultPtr);
}

Tcl_Obj *tjson_TreeToJson(Tcl_Interp *interp, cJSON *item) {
    char *buf = cJSON_PrintUnformatted(item);
    Tcl_Obj *resultPtr = Tcl_NewStringObj(buf, -1);
    free(buf);
    return resultPtr;
}

Tcl_Obj *tjson_TreeToPrettyJson(Tcl_Interp *interp, cJSON *item) {
    char *buf = cJSON_Print(item);
    Tcl_Obj *resultPtr = Tcl_NewStringObj(buf, -1);
    free(buf);
    return resultPtr;
}

static int tjson_ToJsonCmd(ClientData  clientData, Tcl_Interp *interp, int objc, Tcl_Obj * const objv[] ) {
    DBG(fprintf(stderr, "ToJsonCmd\n"));
    CheckArgs(2,2,1,"handle");

    const char *handle = Tcl_GetString(objv[1]);
    cJSON *root_structure = tjson_GetInternalFromNode(handle);
    Tcl_Obj *resultPtr = tjson_TreeToJson(interp, root_structure);
    Tcl_SetObjResult(interp, resultPtr);
    return TCL_OK;
}

static int tjson_ToPrettyJsonCmd(ClientData  clientData, Tcl_Interp *interp, int objc, Tcl_Obj * const objv[] ) {
    DBG(fprintf(stderr, "ToPrettyJsonCmd\n"));
    CheckArgs(2,2,1,"handle");

    const char *handle = Tcl_GetString(objv[1]);
    cJSON *root_structure = tjson_GetInternalFromNode(handle);
    Tcl_Obj *resultPtr = tjson_TreeToPrettyJson(interp, root_structure);
    Tcl_SetObjResult(interp, resultPtr);
    return TCL_OK;
}

static int serialize(Tcl_Interp *interp, Tcl_Obj *specPtr, Tcl_Obj *resultPtr);

static int serialize_list(Tcl_Interp *interp, Tcl_Obj *listPtr, Tcl_Obj *resultPtr) {
    Tcl_AppendStringsToObj(resultPtr, "[", NULL);
    int listLength;
    Tcl_ListObjLength(interp, listPtr, &listLength);
    int first = 1;
    for (int i = 0; i < listLength; i++) {
        if (!first) {
            Tcl_AppendStringsToObj(resultPtr, ", ", NULL);
        } else {
            first = 0;
        }
        Tcl_Obj *elemSpecPtr;
        Tcl_ListObjIndex(interp, listPtr, i, &elemSpecPtr);
        int ret = serialize(interp, elemSpecPtr, resultPtr);
        if (ret) {
            return ret;
        }
    }
    Tcl_AppendStringsToObj(resultPtr, "]", NULL);
    return 0;
}

static int serialize_map(Tcl_Interp *interp, Tcl_Obj *dictPtr, Tcl_Obj *resultPtr) {
    Tcl_AppendStringsToObj(resultPtr, "{", NULL);
    Tcl_DictSearch search;
    Tcl_Obj *key, *elemSpecPtr;
    int done;
    if (Tcl_DictObjFirst(interp, dictPtr, &search,
                         &key, &elemSpecPtr, &done) != TCL_OK) {
        return 3; // invalid dict
    }
    int first = 1;
    for (; !done; Tcl_DictObjNext(&search, &key, &elemSpecPtr, &done)) {
        if (!first) {
            Tcl_AppendStringsToObj(resultPtr, ", ", NULL);
        } else {
            first = 0;
        }
        Tcl_AppendStringsToObj(resultPtr, "\"", tjson_EscapeJsonString(key), "\": ", NULL);
        int ret = serialize(interp, elemSpecPtr, resultPtr);
        if (ret) {
            return ret;
        }
    }
    Tcl_DictObjDone(&search);
    Tcl_AppendStringsToObj(resultPtr, "}", NULL);
    return 0;
}

static int serialize(Tcl_Interp *interp, Tcl_Obj *specPtr, Tcl_Obj *resultPtr) {
    int length;
    Tcl_ListObjLength(interp, specPtr, &length);
    if (length != 2) {
        return 1;  /* invalid spec length */
    }
    Tcl_Obj *typePtr, *valuePtr;
    Tcl_ListObjIndex(interp, specPtr, 0, &typePtr);
    Tcl_ListObjIndex(interp, specPtr, 1, &valuePtr);
    int typeLength;
    const char *type = Tcl_GetStringFromObj(typePtr, &typeLength);
    switch(type[0]) {
        case 'S':
            Tcl_AppendStringsToObj(resultPtr, "\"", tjson_EscapeJsonString(valuePtr), "\"", NULL);
            break;
        case 'N':
            Tcl_AppendStringsToObj(resultPtr, Tcl_GetString(valuePtr), NULL);
            break;
        case 'B':
            if (typeLength == 1) {
                // TODO
            } else if (0 == strcmp("BOOL", type)) {
                int flag;
                Tcl_GetBooleanFromObj(interp, valuePtr, &flag);
                Tcl_AppendStringsToObj(resultPtr, flag ? "true" : "false", NULL);
            }
            break;
        case 'M':
            serialize_map(interp, valuePtr, resultPtr);
            break;
        case 'L':
            serialize_list(interp, valuePtr, resultPtr);
            break;
        default:
            return 2;  /* invalid type */
    }
    return 0;
}

static int tjson_EscapeJsonStringCmd(ClientData  clientData, Tcl_Interp *interp, int objc, Tcl_Obj * const objv[] ) {
    DBG(fprintf(stderr, "EscapeJsonStringCmd\n"));
    CheckArgs(2, 2, 1, "string");

    const char *escaped = tjson_EscapeJsonString(objv[1]);
    Tcl_SetObjResult(interp, Tcl_NewStringObj(escaped, -1));
    return TCL_OK;
}

static void tjson_ExitHandler(ClientData unused) {
}


void tjson_InitModule() {
    if (!tjson_ModuleInitialized) {
        Tcl_InitHashTable(&tjson_NodeToInternal_HT, TCL_STRING_KEYS);
        Tcl_CreateThreadExitHandler(tjson_ExitHandler, NULL);
        tjson_ModuleInitialized = 1;
    }
}

int Tjson_Init(Tcl_Interp *interp) {
    if (Tcl_InitStubs(interp, "8.6", 0) == NULL) {
        return TCL_ERROR;
    }

    tjson_InitModule();

    Tcl_CreateNamespace(interp, "::tjson", NULL, NULL);
    Tcl_CreateObjCommand(interp, "::tjson::json_to_typed", tjson_JsonToTypedCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "::tjson::json_to_simple", tjson_JsonToSimpleCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "::tjson::typed_to_json", tjson_ToJsonCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "::tjson::escape_json_string", tjson_EscapeJsonStringCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "::tjson::parse", tjson_ParseCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "::tjson::destroy", tjson_DestroyCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "::tjson::size", tjson_SizeCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "::tjson::add_item_to_object", tjson_AddItemToObjectCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "::tjson::add_item_to_array", tjson_AddItemToArrayCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "::tjson::insert_item_in_array", tjson_InsertItemInArrayCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "::tjson::to_simple", tjson_ToSimpleCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "::tjson::to_typed", tjson_ToTypedCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "::tjson::to_json", tjson_ToJsonCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "::tjson::to_pretty_json", tjson_ToPrettyJsonCmd, NULL, NULL);

    return Tcl_PkgProvide(interp, "tjson", XSTR(VERSION));
}

#ifdef USE_NAVISERVER
int Ns_ModuleInit(const char *server, const char *module) {
    Ns_TclRegisterTrace(server, (Ns_TclTraceProc *) Tjson_Init, server, NS_TCL_TRACE_CREATE);
    return NS_OK;
}
#endif
