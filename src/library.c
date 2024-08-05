/**
 * Copyright Jerily LTD. All Rights Reserved.
 * SPDX-FileCopyrightText: 2023 Neofytos Dimitriou (neo@jerily.cy)
 * SPDX-License-Identifier: MIT.
 */
#include "library.h"
#include "cJSON/cJSON.h"
#include "jsonpath/jsonpath.h"
#include "custom_triple_notation/custom_triple_notation.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

#ifndef TCL_SIZE_MAX
typedef int Tcl_Size;
# define Tcl_GetSizeIntFromObj Tcl_GetIntFromObj
# define Tcl_NewSizeIntObj Tcl_NewIntObj
# define TCL_SIZE_MAX      INT_MAX
# define TCL_SIZE_MODIFIER ""
#endif

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

#define SetResult(str) Tcl_ResetResult(interp); \
                     Tcl_SetStringObj(Tcl_GetObjResult(interp), (str), -1)

#define CMD_NAME(s, internal) sprintf((s), "_TJSON_%p", (internal))

#define SP " "
#define NL "\n"
#define LBRACKET "["
#define RBRACKET "]"
#define LBRACE "{"
#define RBRACE "}"
#define COMMA ","

static int tjson_ModuleInitialized;

static Tcl_HashTable tjson_NodeToInternal_HT;
static Tcl_Mutex tjson_NodeToInternal_HT_Mutex;

typedef struct {
    Tcl_Interp *interp;
    char *handle;
    char *varname;
    cJSON *item;
} tjson_trace_t;

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

void tjson_Unregister(cJSON *internal) {
    DBG(fprintf(stderr, "Unregister cJSON %p\n", internal));
    char name[80];
    CMD_NAME(name, internal);
    tjson_UnregisterNode(name);
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

    double d;
    Tcl_Obj *listPtr;
    Tcl_Obj *dictPtr;
    switch ((item->type) & 0xFF)
    {
        case cJSON_NULL:
            return Tcl_NewStringObj("", 0);
        case cJSON_False:
            return Tcl_NewBooleanObj(0);
        case cJSON_True:
            return Tcl_NewBooleanObj(1);
        case cJSON_Number:
            d = item->valuedouble;
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
            listPtr = Tcl_NewListObj(0, NULL);
            cJSON *current_element = item->child;
            while (current_element != NULL) {
                Tcl_ListObjAppendElement(interp, listPtr, tjson_TreeToSimple(interp, current_element));
                current_element = current_element->next;
            }
            return listPtr;
        case cJSON_Object:
            dictPtr = Tcl_NewDictObj();
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

    double d;
    Tcl_Obj *resultPtr = Tcl_NewListObj(0, NULL);
    Tcl_Obj *listPtr;
    Tcl_Obj *dictPtr;
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
            d = item->valuedouble;
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
            listPtr = Tcl_NewListObj(0, NULL);
            cJSON *current_element = item->child;
            while (current_element != NULL) {
                Tcl_ListObjAppendElement(interp, listPtr, tjson_TreeToTyped(interp, current_element));
                current_element = current_element->next;
            }
            Tcl_ListObjAppendElement(interp, resultPtr, Tcl_NewStringObj("L", 1));
            Tcl_ListObjAppendElement(interp, resultPtr, listPtr);
            return resultPtr;
        case cJSON_Object:
            dictPtr = Tcl_NewDictObj();
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
    DBG(fprintf(stderr, "JsonToTypedCmd\n"));
    CheckArgs(2,2,1,"json");

    Tcl_Size length;
    const char *json = Tcl_GetStringFromObj(objv[1], &length);

    if (length > 0) {
        cJSON *root_structure = cJSON_ParseWithLength(json, length);
        Tcl_Obj *resultPtr = tjson_TreeToTyped(interp, root_structure);
        cJSON_Delete(root_structure);
        Tcl_SetObjResult(interp, resultPtr);
    }

    return TCL_OK;
}

static int tjson_JsonToSimpleCmd(ClientData  clientData, Tcl_Interp *interp, int objc, Tcl_Obj * const objv[] ) {
    DBG(fprintf(stderr, "JsonToSimpleCmd\n"));
    CheckArgs(2,2,1,"json");

    Tcl_Size length;
    const char *json = Tcl_GetStringFromObj(objv[1], &length);

    if (length > 0) {
        cJSON *root_structure = cJSON_ParseWithLength(json, length);
        if (root_structure) {
            Tcl_Obj *resultPtr = tjson_TreeToSimple(interp, root_structure);
            cJSON_Delete(root_structure);
            Tcl_SetObjResult(interp, resultPtr);
        } else {
            Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid json", -1));
            return TCL_ERROR;
        }
    }
    return TCL_OK;
}

char *tjson_VarTraceProc(ClientData clientData, Tcl_Interp *interp, const char *name1, const char *name2, int flags) {
    tjson_trace_t *trace = (tjson_trace_t *) clientData;
    if (trace->item == NULL) {
        DBG(fprintf(stderr, "VarTraceProc: node has been deleted\n"));
        if (!Tcl_InterpDeleted(trace->interp)) {
            Tcl_UntraceVar(trace->interp, trace->varname, TCL_TRACE_WRITES|TCL_TRACE_UNSETS,
                           (Tcl_VarTraceProc*) tjson_VarTraceProc,
                           (ClientData) clientData);
        }
        Tcl_Free((char *) trace->varname);
        Tcl_Free((char *) trace->handle);
        Tcl_Free((char *) trace);
        return NULL;
    }
    if (flags & TCL_TRACE_WRITES) {
        DBG(fprintf(stderr, "VarTraceProc: TCL_TRACE_WRITES\n"));
        Tcl_SetVar2(trace->interp, name1, name2, trace->handle, TCL_LEAVE_ERR_MSG);
        return "var is read-only";
    }
    if (flags & TCL_TRACE_UNSETS) {
        DBG(fprintf(stderr, "VarTraceProc: TCL_TRACE_UNSETS\n"));
        if (tjson_UnregisterNode(trace->handle)) {
            cJSON_Delete(trace->item);
        }
        Tcl_Free((char *) trace->varname);
        Tcl_Free((char *) trace->handle);
        Tcl_Free((char *) trace);
    }
    return NULL;
}

static char *tjson_strndup(const char *s, size_t n) {
    if (s == NULL) {
        return NULL;
    }
    size_t l = strnlen(s, n);
    char *result = (char *) Tcl_Alloc(l + 1);
    if (result == NULL) {
        return NULL;
    }
    memcpy(result, s, l);
    result[l] = '\0';
    return result;
}

static int tjson_ParseCmd(ClientData  clientData, Tcl_Interp *interp, int objc, Tcl_Obj * const objv[] ) {
    DBG(fprintf(stderr, "ParseCmd\n"));
    CheckArgs(2,3,1,"json ?varname?");

    Tcl_Size length;
    const char *json = Tcl_GetStringFromObj(objv[1], &length);
    if (length == 0) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("empty json", -1));
        return TCL_ERROR;
    }
    cJSON *root_structure = cJSON_ParseWithLength(json, length);
    if (!root_structure) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid json", -1));
        return TCL_ERROR;
    }

    char handle[80];
    CMD_NAME(handle, root_structure);
    tjson_RegisterNode(handle, root_structure);

    if (objc == 3) {
        tjson_trace_t *trace = (tjson_trace_t *) Tcl_Alloc(sizeof(tjson_trace_t));
        trace->interp = interp;
        trace->varname = tjson_strndup(Tcl_GetString(objv[2]), 80);
        trace->handle = tjson_strndup(handle, 80);
        trace->item = root_structure;
        const char *objVar = Tcl_GetString(objv[2]);
        Tcl_UnsetVar(interp, objVar, 0);
        Tcl_SetVar  (interp, objVar, handle, 0);
        Tcl_TraceVar(interp,objVar,TCL_TRACE_WRITES|TCL_TRACE_UNSETS,
                     (Tcl_VarTraceProc*) tjson_VarTraceProc,
                     (ClientData) trace);
    }

    Tcl_SetObjResult(interp, Tcl_NewStringObj(handle, -1));
    return TCL_OK;
}

static int tjson_DestroyCmd(ClientData  clientData, Tcl_Interp *interp, int objc, Tcl_Obj * const objv[] ) {
    DBG(fprintf(stderr, "DestroyCmd\n"));
    CheckArgs(2,2,1,"handle");

    const char *handle = Tcl_GetString(objv[1]);
    cJSON *item = tjson_GetInternalFromNode(handle);
    if (!item) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("node not found", -1));
        return TCL_ERROR;
    }

    tjson_UnregisterNode(handle);
    // todo: if the node is root
    if (item->prev == NULL && item->next == NULL) {
        cJSON_Delete(item);
    } else {
        SetResult("node is not a root");
        return TCL_ERROR;
    }

    return TCL_OK;
}

static int tjson_SizeCmd(ClientData  clientData, Tcl_Interp *interp, int objc, Tcl_Obj * const objv[] ) {
    DBG(fprintf(stderr, "SizeCmd\n"));
    CheckArgs(2,2,1,"handle");

    const char *handle = Tcl_GetString(objv[1]);
    cJSON *root_structure = tjson_GetInternalFromNode(handle);
    if (!root_structure) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("node not found", -1));
        return TCL_ERROR;
    }

    if (!cJSON_IsArray(root_structure) && !cJSON_IsObject(root_structure)) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("node is not an array or object", -1));
        return TCL_ERROR;
    }

    int size = cJSON_GetArraySize(root_structure);
    Tcl_SetObjResult(interp, Tcl_NewIntObj(size));
    return TCL_OK;
}

static int tjson_CreateItemFromSpec(Tcl_Interp *interp, Tcl_Obj *specPtr, cJSON **item) {
    // "specPtr" is a list of two elements: type and value
    Tcl_Size length;
    Tcl_ListObjLength(interp, specPtr, &length);
    if (length != 2) {
        Tcl_Obj *resultPtr = Tcl_NewStringObj("invalid spec length: ", -1);
        Tcl_AppendObjToObj(resultPtr, specPtr);
        Tcl_SetObjResult(interp, resultPtr);
        Tcl_IncrRefCount(resultPtr);
        Tcl_DecrRefCount(resultPtr);
        return TCL_ERROR;
    }
    Tcl_Obj *typePtr, *valuePtr;
    Tcl_ListObjIndex(interp, specPtr, 0, &typePtr);
    Tcl_ListObjIndex(interp, specPtr, 1, &valuePtr);

    double value_double;
    cJSON *obj;
    cJSON *arr;
    Tcl_Size typeLength;
    const char *type = Tcl_GetStringFromObj(typePtr, &typeLength);
    switch (type[0]) {
        case 'S':
            *item = cJSON_CreateString(Tcl_GetString(valuePtr));
            return TCL_OK;
        case 'N':
            Tcl_GetDoubleFromObj(interp, valuePtr, &value_double);
            *item = cJSON_CreateNumber(value_double);
            return TCL_OK;
        case 'B':
            if (typeLength == 4 && 0 == strcmp("BOOL", type)) {
                int flag;
                Tcl_GetBooleanFromObj(NULL, valuePtr, &flag);
                *item = cJSON_CreateBool(flag);
                return TCL_OK;
            } else {
                *item = cJSON_CreateNull();
                return TCL_OK;
            }
        case 'M':
            obj = cJSON_CreateObject();
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
            arr = cJSON_CreateArray();
            // iterate "valuePtr" as a list and add each item to the object "arr"
            Tcl_Size listLength;
            Tcl_ListObjLength(interp, valuePtr, &listLength);
            for (Tcl_Size i = 0; i < listLength; i++) {
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

static int tjson_CreateCmd(ClientData  clientData, Tcl_Interp *interp, int objc, Tcl_Obj * const objv[] ) {
    DBG(fprintf(stderr, "CreateCmd\n"));
    CheckArgs(2,3,1,"typed_item_spec ?varname?");

    cJSON *item = NULL;
    if (TCL_OK != tjson_CreateItemFromSpec(interp, objv[1], &item)) {
        return TCL_ERROR;
    }

    char handle[80];
    CMD_NAME(handle, item);
    tjson_RegisterNode(handle, item);

    if (objc == 3) {
        tjson_trace_t *trace = (tjson_trace_t *) Tcl_Alloc(sizeof(tjson_trace_t));
        trace->interp = interp;
        trace->varname = tjson_strndup(Tcl_GetString(objv[2]), 80);
        trace->handle = tjson_strndup(handle, 80);
        trace->item = item;
        const char *objVar = Tcl_GetString(objv[2]);
        Tcl_UnsetVar(interp, objVar, 0);
        Tcl_SetVar  (interp, objVar, handle, 0);
        Tcl_TraceVar(interp,objVar,TCL_TRACE_WRITES|TCL_TRACE_UNSETS,
                     (Tcl_VarTraceProc*) tjson_VarTraceProc,
                     (ClientData) trace);
    }

    Tcl_SetObjResult(interp, Tcl_NewStringObj(handle, -1));
    return TCL_OK;
}

static int tjson_AddItemToObjectCmd(ClientData  clientData, Tcl_Interp *interp, int objc, Tcl_Obj * const objv[] ) {
    DBG(fprintf(stderr, "AddItemToObjectCmd\n"));
    CheckArgs(4,4,1,"handle key typed_item_spec");

    const char *handle = Tcl_GetString(objv[1]);
    cJSON *root_structure = tjson_GetInternalFromNode(handle);
    if (!root_structure) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("node not found", -1));
        return TCL_ERROR;
    }

    if (!cJSON_IsObject(root_structure)) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("node is not an object", -1));
        return TCL_ERROR;
    }

    cJSON *item = NULL;
    if (TCL_OK != tjson_CreateItemFromSpec(interp, objv[3], &item)) {
        return TCL_ERROR;
    }
    if (!cJSON_AddItemToObject(root_structure, Tcl_GetString(objv[2]), item)) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("error while adding item", -1));
        return TCL_ERROR;
    }
    return TCL_OK;
}

static int tjson_ReplaceItemInObjectCmd(ClientData  clientData, Tcl_Interp *interp, int objc, Tcl_Obj * const objv[] ) {
    DBG(fprintf(stderr, "ReplaceItemInObjectCmd\n"));
    CheckArgs(4,4,1,"handle key typed_item_spec");

    const char *handle = Tcl_GetString(objv[1]);
    cJSON *root_structure = tjson_GetInternalFromNode(handle);
    if (!root_structure) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("node not found", -1));
        return TCL_ERROR;
    }

    if (!cJSON_IsObject(root_structure)) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("node is not an object", -1));
        return TCL_ERROR;
    }

    cJSON *item = NULL;
    if (TCL_OK != tjson_CreateItemFromSpec(interp, objv[3], &item)) {
        return TCL_ERROR;
    }
    if (!cJSON_ReplaceItemInObjectCaseSensitive(root_structure, Tcl_GetString(objv[2]), item)) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("error while replacing item", -1));
        return TCL_ERROR;
    }
    return TCL_OK;
}

static int tjson_DeleteItemFromObjectCmd(ClientData  clientData, Tcl_Interp *interp, int objc, Tcl_Obj * const objv[] ) {
    DBG(fprintf(stderr, "ReplaceItemInObjectCmd\n"));
    CheckArgs(3,3,1,"handle key");

    const char *handle = Tcl_GetString(objv[1]);
    cJSON *root_structure = tjson_GetInternalFromNode(handle);
    if (!root_structure) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("node not found", -1));
        return TCL_ERROR;
    }

    if (!cJSON_IsObject(root_structure)) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("node is not an object", -1));
        return TCL_ERROR;
    }

    cJSON_DeleteItemFromObjectCaseSensitive(root_structure, Tcl_GetString(objv[2]));
    return TCL_OK;
}

static int tjson_GetObjectItemCmd(ClientData  clientData, Tcl_Interp *interp, int objc, Tcl_Obj * const objv[] ) {
    DBG(fprintf(stderr, "GetObjectItemCmd\n"));
    CheckArgs(3,3,1,"handle key");

    const char *handle = Tcl_GetString(objv[1]);
    cJSON *root_structure = tjson_GetInternalFromNode(handle);
    if (!root_structure) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("node not found", -1));
        return TCL_ERROR;
    }

    if (!cJSON_IsObject(root_structure)) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("node is not an object", -1));
        return TCL_ERROR;
    }

    cJSON *item = cJSON_GetObjectItemCaseSensitive(root_structure, Tcl_GetString(objv[2]));
    if (item == NULL) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("key not found", -1));
        return TCL_ERROR;
    }

    char item_handle[80];
    CMD_NAME(item_handle, item);
    tjson_RegisterNode(item_handle, item);
    // IMPORTANT: mark the node to unregister when cJSON_Delete is called
    item->flags |= VISIBLE_IN_TCL;

    Tcl_SetObjResult(interp, Tcl_NewStringObj(item_handle, -1));
    return TCL_OK;
}

static int tjson_HasObjectItemCmd(ClientData  clientData, Tcl_Interp *interp, int objc, Tcl_Obj * const objv[] ) {
    DBG(fprintf(stderr, "HasObjectItemCmd\n"));
    CheckArgs(3,3,1,"handle key");

    const char *handle = Tcl_GetString(objv[1]);
    cJSON *root_structure = tjson_GetInternalFromNode(handle);
    if (!root_structure) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("node not found", -1));
        return TCL_ERROR;
    }

    if (!cJSON_IsObject(root_structure)) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("node is not an object", -1));
        return TCL_ERROR;
    }

    cJSON_bool exists = cJSON_HasObjectItem(root_structure, Tcl_GetString(objv[2]));

    Tcl_SetObjResult(interp, Tcl_NewBooleanObj(exists));
    return TCL_OK;
}

static int tjson_AddItemToArrayCmd(ClientData  clientData, Tcl_Interp *interp, int objc, Tcl_Obj * const objv[] ) {
    DBG(fprintf(stderr, "AddItemToArrayCmd\n"));
    CheckArgs(3,3,1,"handle typed_item_spec");

    const char *handle = Tcl_GetString(objv[1]);
    cJSON *root_structure = tjson_GetInternalFromNode(handle);
    if (!root_structure) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("node not found", -1));
        return TCL_ERROR;
    }

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
    if (!root_structure) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("node not found", -1));
        return TCL_ERROR;
    }

    if (!cJSON_IsArray(root_structure)) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("node is not an array", -1));
        return TCL_ERROR;
    }

    int index;
    if (TCL_OK != Tcl_GetIntFromObj(interp, objv[2], &index)) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid index", -1));
        return TCL_ERROR;
    }

    if (index < 0 || index >= cJSON_GetArraySize(root_structure)) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("index out of bounds", -1));
        return TCL_ERROR;
    }

    cJSON *item = NULL;
    if (TCL_OK != tjson_CreateItemFromSpec(interp, objv[3], &item)) {
        return TCL_ERROR;
    }
    cJSON_InsertItemInArray(root_structure, index, item);

    return TCL_OK;
}

static int tjson_ReplaceItemInArrayCmd(ClientData  clientData, Tcl_Interp *interp, int objc, Tcl_Obj * const objv[] ) {
    DBG(fprintf(stderr, "ReplaceItemInArrayCmd\n"));
    CheckArgs(4,4,1,"handle index typed_item_spec");

    const char *handle = Tcl_GetString(objv[1]);
    cJSON *root_structure = tjson_GetInternalFromNode(handle);
    if (!root_structure) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("node not found", -1));
        return TCL_ERROR;
    }

    if (!cJSON_IsArray(root_structure)) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("node is not an array", -1));
        return TCL_ERROR;
    }

    int index;
    if (TCL_OK != Tcl_GetIntFromObj(interp, objv[2], &index)) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid index", -1));
        return TCL_ERROR;
    }

    if (index < 0 || index >= cJSON_GetArraySize(root_structure)) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("index out of bounds", -1));
        return TCL_ERROR;
    }

    cJSON *item = NULL;
    if (TCL_OK != tjson_CreateItemFromSpec(interp, objv[3], &item)) {
        return TCL_ERROR;
    }
    cJSON_ReplaceItemInArray(root_structure, index, item);

    return TCL_OK;
}

static int tjson_DeleteItemFromArrayCmd(ClientData  clientData, Tcl_Interp *interp, int objc, Tcl_Obj * const objv[] ) {
    DBG(fprintf(stderr, "DeleteItemFromArrayCmd\n"));
    CheckArgs(3,3,1,"handle index");

    const char *handle = Tcl_GetString(objv[1]);
    cJSON *root_structure = tjson_GetInternalFromNode(handle);
    if (!root_structure) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("node not found", -1));
        return TCL_ERROR;
    }

    if (!cJSON_IsArray(root_structure)) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("node is not an array", -1));
        return TCL_ERROR;
    }

    int index;
    if (TCL_OK != Tcl_GetIntFromObj(interp, objv[2], &index)) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid index", -1));
        return TCL_ERROR;
    }

    if (index < 0 || index >= cJSON_GetArraySize(root_structure)) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("index out of bounds", -1));
        return TCL_ERROR;
    }

    cJSON_DeleteItemFromArray(root_structure, index);

    return TCL_OK;
}

static int tjson_GetArrayItemCmd(ClientData  clientData, Tcl_Interp *interp, int objc, Tcl_Obj * const objv[] ) {
    DBG(fprintf(stderr, "GetArrayItemCmd\n"));
    CheckArgs(3,3,1,"handle index");

    const char *handle = Tcl_GetString(objv[1]);
    cJSON *root_structure = tjson_GetInternalFromNode(handle);
    if (!root_structure) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("node not found", -1));
        return TCL_ERROR;
    }

    if (!cJSON_IsArray(root_structure) && !cJSON_IsObject(root_structure)) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("node is not an array or object", -1));
        return TCL_ERROR;
    }

    int index;
    if (TCL_OK != Tcl_GetIntFromObj(interp, objv[2], &index)) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid index", -1));
        return TCL_ERROR;
    }

    if (index < 0 || index >= cJSON_GetArraySize(root_structure)) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("index out of bounds", -1));
        return TCL_ERROR;
    }

    cJSON *item = cJSON_GetArrayItem(root_structure, index);
    char item_handle[80];
    CMD_NAME(item_handle, item);
    tjson_RegisterNode(item_handle, item);
    // IMPORTANT: mark the node to unregister when cJSON_Delete is called
    item->flags |= VISIBLE_IN_TCL;

    Tcl_SetObjResult(interp, Tcl_NewStringObj(item_handle, -1));

    return TCL_OK;
}

static int tjson_GetStringCmd(ClientData  clientData, Tcl_Interp *interp, int objc, Tcl_Obj * const objv[] ) {
    DBG(fprintf(stderr, "GetStringCmd\n"));
    CheckArgs(2,2,1,"handle");

    const char *handle = Tcl_GetString(objv[1]);
    cJSON *root_structure = tjson_GetInternalFromNode(handle);
    if (!root_structure) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("node not found", -1));
        return TCL_ERROR;
    }

    Tcl_Obj *resultPtr = Tcl_NewStringObj(root_structure->string, -1);
    Tcl_SetObjResult(interp, resultPtr);
    return TCL_OK;
}

static int tjson_GetValueStringCmd(ClientData  clientData, Tcl_Interp *interp, int objc, Tcl_Obj * const objv[] ) {
    DBG(fprintf(stderr, "GetValueStringCmd\n"));
    CheckArgs(2,2,1,"handle");

    const char *handle = Tcl_GetString(objv[1]);
    cJSON *root_structure = tjson_GetInternalFromNode(handle);
    if (!root_structure) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("node not found", -1));
        return TCL_ERROR;
    }

    Tcl_Obj *resultPtr = Tcl_NewStringObj(root_structure->valuestring, -1);
    Tcl_SetObjResult(interp, resultPtr);
    return TCL_OK;
}


static int tjson_IsNumberCmd(ClientData  clientData, Tcl_Interp *interp, int objc, Tcl_Obj * const objv[] ) {
    DBG(fprintf(stderr, "IsNumberCmd\n"));
    CheckArgs(2,2,1,"handle");

    const char *handle = Tcl_GetString(objv[1]);
    cJSON *root_structure = tjson_GetInternalFromNode(handle);
    if (!root_structure) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("node not found", -1));
        return TCL_ERROR;
    }

    Tcl_Obj *resultPtr = Tcl_NewBooleanObj(cJSON_IsNumber(root_structure));
    Tcl_SetObjResult(interp, resultPtr);
    return TCL_OK;
}

static int tjson_IsBoolCmd(ClientData  clientData, Tcl_Interp *interp, int objc, Tcl_Obj * const objv[] ) {
    DBG(fprintf(stderr, "IsBoolCmd\n"));
    CheckArgs(2,2,1,"handle");

    const char *handle = Tcl_GetString(objv[1]);
    cJSON *root_structure = tjson_GetInternalFromNode(handle);
    if (!root_structure) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("node not found", -1));
        return TCL_ERROR;
    }

    Tcl_Obj *resultPtr = Tcl_NewBooleanObj(cJSON_IsBool(root_structure));
    Tcl_SetObjResult(interp, resultPtr);
    return TCL_OK;
}

static int tjson_IsStringCmd(ClientData  clientData, Tcl_Interp *interp, int objc, Tcl_Obj * const objv[] ) {
    DBG(fprintf(stderr, "IsStringCmd\n"));
    CheckArgs(2,2,1,"handle");

    const char *handle = Tcl_GetString(objv[1]);
    cJSON *root_structure = tjson_GetInternalFromNode(handle);
    if (!root_structure) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("node not found", -1));
        return TCL_ERROR;
    }

    Tcl_Obj *resultPtr = Tcl_NewBooleanObj(cJSON_IsString(root_structure));
    Tcl_SetObjResult(interp, resultPtr);
    return TCL_OK;
}

static int tjson_IsNullCmd(ClientData  clientData, Tcl_Interp *interp, int objc, Tcl_Obj * const objv[] ) {
    DBG(fprintf(stderr, "IsNullCmd\n"));
    CheckArgs(2,2,1,"handle");

    const char *handle = Tcl_GetString(objv[1]);
    cJSON *root_structure = tjson_GetInternalFromNode(handle);
    if (!root_structure) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("node not found", -1));
        return TCL_ERROR;
    }

    Tcl_Obj *resultPtr = Tcl_NewBooleanObj(cJSON_IsNull(root_structure));
    Tcl_SetObjResult(interp, resultPtr);
    return TCL_OK;
}

static int tjson_IsObjectCmd(ClientData  clientData, Tcl_Interp *interp, int objc, Tcl_Obj * const objv[] ) {
    DBG(fprintf(stderr, "ToSimpleCmd\n"));
    CheckArgs(2,2,1,"handle");

    const char *handle = Tcl_GetString(objv[1]);
    cJSON *root_structure = tjson_GetInternalFromNode(handle);
    if (!root_structure) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("node not found", -1));
        return TCL_ERROR;
    }

    Tcl_Obj *resultPtr = Tcl_NewBooleanObj(cJSON_IsObject(root_structure));
    Tcl_SetObjResult(interp, resultPtr);
    return TCL_OK;
}

static int tjson_IsArrayCmd(ClientData  clientData, Tcl_Interp *interp, int objc, Tcl_Obj * const objv[] ) {
    DBG(fprintf(stderr, "IsArrayCmd\n"));
    CheckArgs(2,2,1,"handle");

    const char *handle = Tcl_GetString(objv[1]);
    cJSON *root_structure = tjson_GetInternalFromNode(handle);
    if (!root_structure) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("node not found", -1));
        return TCL_ERROR;
    }

    Tcl_Obj *resultPtr = Tcl_NewBooleanObj(cJSON_IsArray(root_structure));
    Tcl_SetObjResult(interp, resultPtr);
    return TCL_OK;
}


static int tjson_GetChildItemsCmd(ClientData  clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[]) {
    DBG(fprintf(stderr, "GetChildNodesCmd\n"));
    CheckArgs(2,2,1,"handle");

    const char *handle = Tcl_GetString(objv[1]);
    cJSON *root_structure = tjson_GetInternalFromNode(handle);
    if (!root_structure) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("node not found", -1));
        return TCL_ERROR;
    }

    if (!cJSON_IsArray(root_structure) && !cJSON_IsObject(root_structure)) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("node is not an array or object", -1));
        return TCL_ERROR;
    }

    Tcl_Obj *list_ptr = Tcl_NewListObj(0, NULL);
    Tcl_IncrRefCount(list_ptr);
    cJSON *element;
    cJSON_ArrayForEach(element, root_structure) {
        char item_handle[80];
        CMD_NAME(item_handle, element);
        tjson_RegisterNode(item_handle, element);
        // IMPORTANT: mark the node to unregister when cJSON_Delete is called
        element->flags |= VISIBLE_IN_TCL;

        if (TCL_OK != Tcl_ListObjAppendElement(interp, list_ptr, Tcl_NewStringObj(item_handle, -1))) {
            Tcl_DecrRefCount(list_ptr);
            Tcl_SetObjResult(interp, Tcl_NewStringObj("error while appending to list", -1));
            return TCL_ERROR;
        }
    }

    Tcl_SetObjResult(interp, list_ptr);
    Tcl_DecrRefCount(list_ptr);
    return TCL_OK;
}


static int tjson_ToSimpleCmd(ClientData  clientData, Tcl_Interp *interp, int objc, Tcl_Obj * const objv[] ) {
    DBG(fprintf(stderr, "ToSimpleCmd\n"));
    CheckArgs(2,2,1,"handle");

    const char *handle = Tcl_GetString(objv[1]);
    cJSON *root_structure = tjson_GetInternalFromNode(handle);
    if (!root_structure) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("node not found", -1));
        return TCL_ERROR;
    }

    Tcl_Obj *resultPtr = tjson_TreeToSimple(interp, root_structure);
    Tcl_SetObjResult(interp, resultPtr);
    return TCL_OK;
}

static int tjson_ToTypedCmd(ClientData  clientData, Tcl_Interp *interp, int objc, Tcl_Obj * const objv[] ) {
    DBG(fprintf(stderr, "ToTypedCmd\n"));
    CheckArgs(2,2,1,"handle");

    const char *handle = Tcl_GetString(objv[1]);
    cJSON *root_structure = tjson_GetInternalFromNode(handle);
    if (!root_structure) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("node not found", -1));
        return TCL_ERROR;
    }

    Tcl_Obj *resultPtr = tjson_TreeToTyped(interp, root_structure);
    Tcl_SetObjResult(interp, resultPtr);
    return TCL_OK;
}

static int tjson_EscapeJsonString(Tcl_Obj *objPtr, Tcl_DString *dsPtr) {
    Tcl_Size length;
    const char *str = Tcl_GetStringFromObj(objPtr, &length);
    // loop through each character of the input string
    for (Tcl_Size i = 0; i < length; i++) {
        unsigned char c = str[i];
        switch (c) {
            case '"':
                Tcl_DStringAppend(dsPtr, "\\\"", 2);
                break;
            case '\\':
                Tcl_DStringAppend(dsPtr, "\\\\", 2);
                break;
            case '\b':
                Tcl_DStringAppend(dsPtr, "\\b", 2);
                break;
            case '\f':
                Tcl_DStringAppend(dsPtr, "\\f", 2);
                break;
            case '\n':
                Tcl_DStringAppend(dsPtr, "\\n", 2);
                break;
            case '\r':
                Tcl_DStringAppend(dsPtr, "\\r", 2);
                break;
            case '\t':
                Tcl_DStringAppend(dsPtr, "\\t", 2);
                break;
            default:
                if (c < 32) {
                    Tcl_DStringAppend(dsPtr, "\\u00", 4);
                    char hex[3];
                    sprintf(hex, "%02x", c);
                    Tcl_DStringAppend(dsPtr, hex, 2);
                } else {
                    char tempstr[2];
                    tempstr[0] = c;
                    tempstr[1] = '\0';
                    Tcl_DStringAppend(dsPtr, tempstr, 1);
                }
        }
    }
    return TCL_OK;
}

static int tjson_TreeToJson(Tcl_Interp *interp, cJSON *item, int num_spaces, Tcl_DString *dsPtr) {
    double d;
    switch ((item->type) & 0xFF)
    {
        case cJSON_NULL:
            Tcl_DStringAppend(dsPtr, "null", 4);
            return TCL_OK;
        case cJSON_False:
            Tcl_DStringAppend(dsPtr, "false", 5);
            return TCL_OK;
        case cJSON_True:
            Tcl_DStringAppend(dsPtr, "true", 4);
            return TCL_OK;
        case cJSON_Number:
            d = item->valuedouble;
            if (isnan(d) || isinf(d)) {
                Tcl_DStringAppend(dsPtr, "null", 4);
                return TCL_OK;
            } else if(d == (double)item->valueint) {
                Tcl_Size intstr_length;
                Tcl_Obj *intObjPtr = Tcl_NewIntObj(item->valueint);
                Tcl_IncrRefCount(intObjPtr);
                const char *intstr = Tcl_GetStringFromObj(intObjPtr, &intstr_length);
                Tcl_DStringAppend(dsPtr, intstr, intstr_length);
                Tcl_DecrRefCount(intObjPtr);
                return TCL_OK;
            } else {
                Tcl_Size doublestr_length;
                Tcl_Obj *doubleObjPtr = Tcl_NewDoubleObj(item->valuedouble);
                Tcl_IncrRefCount(doubleObjPtr);
                const char *doublestr = Tcl_GetStringFromObj(doubleObjPtr, &doublestr_length);
                Tcl_DStringAppend(dsPtr, doublestr, doublestr_length);
                Tcl_DecrRefCount(doubleObjPtr);
                return TCL_OK;
            }
        case cJSON_Raw:
        {
            if (item->valuestring == NULL)
            {
                Tcl_DStringAppend(dsPtr, "\"\"", 2);
                return TCL_OK;
            }
            Tcl_DStringAppend(dsPtr, "\"", 1);
            Tcl_Obj *strObjPtr = Tcl_NewStringObj(item->valuestring, -1);
            Tcl_IncrRefCount(strObjPtr);
            tjson_EscapeJsonString(strObjPtr, dsPtr);
            Tcl_DStringAppend(dsPtr, "\"", 1);
            Tcl_DecrRefCount(strObjPtr);
            return TCL_OK;
        }

        case cJSON_String:
            Tcl_DStringAppend(dsPtr, "\"", 1);
            Tcl_Obj *strObjPtr = Tcl_NewStringObj(item->valuestring, -1);
            Tcl_IncrRefCount(strObjPtr);
            tjson_EscapeJsonString(strObjPtr, dsPtr);
            Tcl_DStringAppend(dsPtr, "\"", 1);
            Tcl_DecrRefCount(strObjPtr);
            return TCL_OK;
        case cJSON_Array:
            Tcl_DStringAppend(dsPtr, LBRACKET, 1);
            if (num_spaces) {
                Tcl_DStringAppend(dsPtr, NL, 1);
            }
            cJSON *current_element = item->child;
            int first_array_element = 1;
            while (current_element != NULL) {
                if (first_array_element) {
                    first_array_element = 0;
                } else {
                    Tcl_DStringAppend(dsPtr, COMMA, 1);
                    if (num_spaces) {
                        Tcl_DStringAppend(dsPtr, NL, 1);
                    }
                }
                if (num_spaces) {
                    for (int i = 0; i < num_spaces; i++) {
                        Tcl_DStringAppend(dsPtr, SP, 1);
                    }
                }
                if (TCL_OK != tjson_TreeToJson(interp, current_element, num_spaces > 0 ? num_spaces + 2 : 0, dsPtr)) {
                    return TCL_ERROR;
                }
                current_element = current_element->next;
            }
            if (num_spaces) {
                Tcl_DStringAppend(dsPtr, NL, 1);
                for (int i = 0; i < num_spaces - 2; i++) {
                    Tcl_DStringAppend(dsPtr, SP, 1);
                }
            }
            Tcl_DStringAppend(dsPtr, RBRACKET, 1);
            return TCL_OK;
        case cJSON_Object:
            Tcl_DStringAppend(dsPtr, LBRACE, 1);
            if (num_spaces) {
                Tcl_DStringAppend(dsPtr, NL, 1);
            }
            cJSON *current_item = item->child;
            int first_object_item = 1;
            while (current_item) {
                if (first_object_item) {
                    first_object_item = 0;
                } else {
                    Tcl_DStringAppend(dsPtr, COMMA, 1);
                    if (num_spaces) {
                        Tcl_DStringAppend(dsPtr, NL, 1);
                    }
                }
                if (num_spaces) {
                    for (int i = 0; i < num_spaces; i++) {
                        Tcl_DStringAppend(dsPtr, SP, 1);
                    }
                }
                Tcl_DStringAppend(dsPtr, "\"", 1);
                Tcl_Obj *strToEscapeObjPtr = Tcl_NewStringObj(current_item->string, -1);
                Tcl_IncrRefCount(strToEscapeObjPtr);
                tjson_EscapeJsonString(strToEscapeObjPtr, dsPtr);
                Tcl_DecrRefCount(strToEscapeObjPtr);
                Tcl_DStringAppend(dsPtr, "\":", 2);
                if (num_spaces) {
                    Tcl_DStringAppend(dsPtr, SP, 1);
                }
                if (TCL_OK != tjson_TreeToJson(interp, current_item, num_spaces > 0 ? num_spaces + 2 : 0, dsPtr)) {
                    return TCL_ERROR;
                }
                current_item = current_item->next;
            }
            if (num_spaces) {
                Tcl_DStringAppend(dsPtr, NL, 1);
                for (int i = 0; i < num_spaces - 2; i++) {
                    Tcl_DStringAppend(dsPtr, SP, 1);
                }
            }
            Tcl_DStringAppend(dsPtr, RBRACE, 1);
            return TCL_OK;
        default:
            Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid type", -1));
            return TCL_ERROR;
    }
}

static int tjson_ToJsonCmd(ClientData  clientData, Tcl_Interp *interp, int objc, Tcl_Obj * const objv[] ) {
    DBG(fprintf(stderr, "ToJsonCmd\n"));
    CheckArgs(2,2,1,"node_handle");

    const char *handle = Tcl_GetString(objv[1]);
    cJSON *root_structure = tjson_GetInternalFromNode(handle);
    if (!root_structure) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("node not found", -1));
        return TCL_ERROR;
    }

    Tcl_DString ds;
    Tcl_DStringInit(&ds);
    if (TCL_OK != tjson_TreeToJson(interp, root_structure, 0, &ds)) {
        Tcl_DStringFree(&ds);
        return TCL_ERROR;
    }
    Tcl_DStringResult(interp, &ds);
    Tcl_DStringFree(&ds);
    return TCL_OK;
}

static int tjson_ToPrettyJsonCmd(ClientData  clientData, Tcl_Interp *interp, int objc, Tcl_Obj * const objv[] ) {
    DBG(fprintf(stderr, "ToPrettyJsonCmd\n"));
    CheckArgs(2,2,1,"handle");

    const char *handle = Tcl_GetString(objv[1]);
    cJSON *root_structure = tjson_GetInternalFromNode(handle);
    if (!root_structure) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("node not found", -1));
        return TCL_ERROR;
    }

    Tcl_DString ds;
    Tcl_DStringInit(&ds);
    if (TCL_OK != tjson_TreeToJson(interp, root_structure, 2, &ds)) {
        Tcl_DStringFree(&ds);
        return TCL_ERROR;
    }
    Tcl_DStringResult(interp, &ds);
    Tcl_DStringFree(&ds);
    return TCL_OK;
}

static int tjson_QueryCmd(ClientData  clientData, Tcl_Interp *interp, int objc, Tcl_Obj * const objv[] ) {
    DBG(fprintf(stderr, "AppendItemToArrayCmd\n"));
    CheckArgs(3, 3, 1, "handle jsonpath");

    const char *handle = Tcl_GetString(objv[1]);
    cJSON *root_structure = tjson_GetInternalFromNode(handle);
    if (!root_structure) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("node not found", -1));
        return TCL_ERROR;
    }

    Tcl_Size length;
    const char *jsonpath = Tcl_GetStringFromObj(objv[2], &length);
    jsonpath_result_t result;
    result.k = 16;
    result.items_length = 0;
    result.items = (cJSON **)malloc(sizeof(cJSON *) * result.k);
    if (TCL_OK != jsonpath_match(interp, jsonpath, length, root_structure, &result)) {
        free(result.items);
        return TCL_ERROR;
    }
    Tcl_Obj *listPtr = Tcl_NewListObj(0, NULL);
    for (int i = 0; i < result.items_length; i++) {
        char item_handle[80];
        CMD_NAME(item_handle, result.items[i]);
        tjson_RegisterNode(item_handle, result.items[i]);
        // IMPORTANT: mark the node to unregister when cJSON_Delete is called
        result.items[i]->flags |= VISIBLE_IN_TCL;
        Tcl_ListObjAppendElement(interp, listPtr, Tcl_NewStringObj(item_handle, -1));
    }
    free(result.items);
    Tcl_SetObjResult(interp, listPtr);
    return TCL_OK;
}

static int serialize(Tcl_Interp *interp, Tcl_Obj *specPtr, Tcl_DString *dsPtr);

static int serialize_list(Tcl_Interp *interp, Tcl_Obj *listPtr, Tcl_DString *dsPtr) {
    Tcl_DStringAppend(dsPtr, LBRACKET, 1);
    Tcl_Size listLength;
    Tcl_ListObjLength(interp, listPtr, &listLength);
    int first = 1;
    for (Tcl_Size i = 0; i < listLength; i++) {
        if (!first) {
            Tcl_DStringAppend(dsPtr, COMMA, 1);
        } else {
            first = 0;
        }
        Tcl_Obj *elemSpecPtr;
        Tcl_ListObjIndex(interp, listPtr, i, &elemSpecPtr);
        if (TCL_OK != serialize(interp, elemSpecPtr, dsPtr)) {
            return TCL_ERROR;
        }
    }
    Tcl_DStringAppend(dsPtr, RBRACKET, 1);
    return TCL_OK;
}

static int serialize_map(Tcl_Interp *interp, Tcl_Obj *dictPtr, Tcl_DString *dsPtr) {
    Tcl_DStringAppend(dsPtr, LBRACE, -1);
    Tcl_DictSearch search;
    Tcl_Obj *key, *elemSpecPtr;
    int done;
    if (Tcl_DictObjFirst(interp, dictPtr, &search,
                         &key, &elemSpecPtr, &done) != TCL_OK) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid dict", -1));
        return TCL_ERROR; // invalid dict
    }
    int first = 1;
    for (; !done; Tcl_DictObjNext(&search, &key, &elemSpecPtr, &done)) {
        if (!first) {
            Tcl_DStringAppend(dsPtr, COMMA, 1);
        } else {
            first = 0;
        }
        Tcl_DStringAppend(dsPtr, "\"", 1);
        tjson_EscapeJsonString(key, dsPtr);
        Tcl_DStringAppend(dsPtr, "\":", 2);
        if (TCL_OK != serialize(interp, elemSpecPtr, dsPtr)) {
            return TCL_ERROR;
        }
    }
    Tcl_DictObjDone(&search);
    Tcl_DStringAppend(dsPtr, RBRACE, 1);
    return TCL_OK;
}

static int serialize(Tcl_Interp *interp, Tcl_Obj *specPtr, Tcl_DString *dsPtr) {
    Tcl_Size length;
    Tcl_ListObjLength(interp, specPtr, &length);
    if (length != 2) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid spec", -1));
        return TCL_ERROR;  /* invalid spec length */
    }
    Tcl_Obj *typePtr, *valuePtr;
    Tcl_ListObjIndex(interp, specPtr, 0, &typePtr);
    Tcl_ListObjIndex(interp, specPtr, 1, &valuePtr);
    Tcl_Size typeLength;
    const char *type = Tcl_GetStringFromObj(typePtr, &typeLength);
    Tcl_Size numstr_length;
    const char *numstr;
    switch(type[0]) {
        case 'S':
            Tcl_DStringAppend(dsPtr, "\"", 1);
            tjson_EscapeJsonString(valuePtr, dsPtr);
            Tcl_DStringAppend(dsPtr, "\"", 1);
            break;
        case 'N':
            numstr = Tcl_GetStringFromObj(valuePtr, &numstr_length);
            Tcl_DStringAppend(dsPtr, numstr, numstr_length);
            break;
        case 'B':
            if (typeLength == 1) {
                // TODO
            } else if (0 == strcmp("BOOL", type)) {
                int flag;
                Tcl_GetBooleanFromObj(interp, valuePtr, &flag);
                Tcl_DStringAppend(dsPtr, flag ? "true" : "false", flag ? 4 : 5);
            }
            break;
        case 'M':
            if (TCL_OK != serialize_map(interp, valuePtr, dsPtr)) {
                return TCL_ERROR;
            }
            break;
        case 'L':
            if (TCL_OK != serialize_list(interp, valuePtr, dsPtr)) {
                return TCL_ERROR;
            }
            break;
        default:
            Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid type in spec", -1));
            return TCL_ERROR;  /* invalid type */
    }
    return TCL_OK;
}


static int tjson_TypedToJsonCmd(ClientData  clientData, Tcl_Interp *interp, int objc, Tcl_Obj * const objv[] ) {
    DBG(fprintf(stderr, "TypedToJsonCmd\n"));
    CheckArgs(2,2,1,"typed_spec");

    Tcl_DString ds;
    Tcl_DStringInit(&ds);
    if (TCL_OK != serialize(interp, objv[1], &ds)) {
        Tcl_DStringFree(&ds);
        return TCL_ERROR;
    }
    Tcl_DStringResult(interp, &ds);
    Tcl_DStringFree(&ds);
    return TCL_OK;
}

static int tjson_EscapeJsonStringCmd(ClientData  clientData, Tcl_Interp *interp, int objc, Tcl_Obj * const objv[] ) {
    DBG(fprintf(stderr, "EscapeJsonStringCmd\n"));
    CheckArgs(2, 2, 1, "string");

    Tcl_DString ds;
    Tcl_DStringInit(&ds);
    tjson_EscapeJsonString(objv[1], &ds);
    Tcl_DStringResult(interp, &ds);
    Tcl_DStringFree(&ds);
    return TCL_OK;
}

static int tjson_CustomToTypedCmd(ClientData  clientData, Tcl_Interp *interp, int objc, Tcl_Obj * const objv[] ) {
    DBG(fprintf(stderr, "CustomToTypedCmd\n"));
    CheckArgs(2, 2, 1, "triple_notation_spec");

    Tcl_Obj *resultPtr = NULL;
    if (TCL_OK != tjson_CustomToTyped(interp, objv[1], &resultPtr)) {
        return TCL_ERROR;
    }
    Tcl_SetObjResult(interp, resultPtr);
    return TCL_OK;
}

static int tjson_TypedToCustomCmd(ClientData  clientData, Tcl_Interp *interp, int objc, Tcl_Obj * const objv[] ) {
    DBG(fprintf(stderr, "TypeToCustomCmd\n"));
    CheckArgs(2, 2, 1, "typed_spec");

    Tcl_Obj *resultPtr = NULL;
    if (TCL_OK != tjson_TypedToCustom(interp, objv[1], &resultPtr)) {
        return TCL_ERROR;
    }
    Tcl_SetObjResult(interp, resultPtr);
    return TCL_OK;
}

static void tjson_ExitHandler(ClientData unused) {
    Tcl_MutexLock(&tjson_NodeToInternal_HT_Mutex);
    Tcl_DeleteHashTable(&tjson_NodeToInternal_HT);
    Tcl_MutexUnlock(&tjson_NodeToInternal_HT_Mutex);
}


void tjson_InitModule() {
    if (!tjson_ModuleInitialized) {
        cJSON_Hooks *hooks = malloc(sizeof(cJSON_Hooks));
        hooks->malloc_fn = NULL;
        hooks->free_fn = NULL;
        hooks->unregister_fn = tjson_Unregister;
        cJSON_InitHooks(hooks);
        free(hooks);
        Tcl_MutexLock(&tjson_NodeToInternal_HT_Mutex);
        Tcl_InitHashTable(&tjson_NodeToInternal_HT, TCL_STRING_KEYS);
        Tcl_MutexUnlock(&tjson_NodeToInternal_HT_Mutex);
        Tcl_CreateThreadExitHandler(tjson_ExitHandler, NULL);
        tjson_ModuleInitialized = 1;
        DBG(fprintf(stderr, "tjson module initialized\n"));
    }
}

#if TCL_MAJOR_VERSION > 8
#define MIN_VERSION "9.0"
#else
#define MIN_VERSION "8.6"
#endif

int Tjson_Init(Tcl_Interp *interp) {

    if (Tcl_InitStubs(interp, MIN_VERSION, 0) == NULL) {
        SetResult("Unable to initialize Tcl stubs");
        return TCL_ERROR;
    }

    tjson_InitModule();

    Tcl_CreateNamespace(interp, "::tjson", NULL, NULL);
    Tcl_CreateObjCommand(interp, "::tjson::json_to_typed", tjson_JsonToTypedCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "::tjson::json_to_simple", tjson_JsonToSimpleCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "::tjson::typed_to_json", tjson_TypedToJsonCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "::tjson::escape_json_string", tjson_EscapeJsonStringCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "::tjson::parse", tjson_ParseCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "::tjson::create", tjson_CreateCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "::tjson::destroy", tjson_DestroyCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "::tjson::size", tjson_SizeCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "::tjson::add_item_to_object", tjson_AddItemToObjectCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "::tjson::replace_item_in_object", tjson_ReplaceItemInObjectCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "::tjson::delete_item_from_object", tjson_DeleteItemFromObjectCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "::tjson::get_object_item", tjson_GetObjectItemCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "::tjson::has_object_item", tjson_HasObjectItemCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "::tjson::add_item_to_array", tjson_AddItemToArrayCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "::tjson::insert_item_in_array", tjson_InsertItemInArrayCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "::tjson::replace_item_in_array", tjson_ReplaceItemInArrayCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "::tjson::delete_item_from_array", tjson_DeleteItemFromArrayCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "::tjson::get_array_item", tjson_GetArrayItemCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "::tjson::get_string", tjson_GetStringCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "::tjson::get_valuestring", tjson_GetValueStringCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "::tjson::is_number", tjson_IsNumberCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "::tjson::is_bool", tjson_IsBoolCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "::tjson::is_object", tjson_IsObjectCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "::tjson::is_array", tjson_IsArrayCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "::tjson::is_string", tjson_IsStringCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "::tjson::is_null", tjson_IsNullCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "::tjson::get_child_items", tjson_GetChildItemsCmd, NULL, NULL);

    Tcl_CreateObjCommand(interp, "::tjson::to_simple", tjson_ToSimpleCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "::tjson::to_typed", tjson_ToTypedCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "::tjson::to_json", tjson_ToJsonCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "::tjson::to_pretty_json", tjson_ToPrettyJsonCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "::tjson::query", tjson_QueryCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "::tjson::custom_to_typed", tjson_CustomToTypedCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "::tjson::typed_to_custom", tjson_TypedToCustomCmd, NULL, NULL);

    return Tcl_PkgProvide(interp, "tjson", XSTR(VERSION));
}

#ifdef USE_NAVISERVER
int Ns_ModuleInit(const char *server, const char *module) {
    Ns_TclRegisterTrace(server, (Ns_TclTraceProc *) Tjson_Init, server, NS_TCL_TRACE_CREATE);
    return NS_OK;
}
#endif
