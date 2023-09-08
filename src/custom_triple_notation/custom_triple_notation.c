#include <tcl.h>
#include <string.h>
#include "custom_triple_notation.h"

// The triple notation is a flat list containing triples of the form
//
//    NAME TYPE VALUE
//
//where "TYPE" might have the following values:
//
//  "array",
//  "binary",
//  "boolean",
//  "int32",
//  "int64",
//  "datetime",
//  "decimal128",
//  "document",
//  "double",
//  "minkey",
//  "maxkey",
//  "null",
//  "oid",
//  "regex",
//  "string",
//  "timestamp",
//  "unknown"

int tjson_CustomToTyped(Tcl_Interp *interp, Tcl_Obj *specPtr, Tcl_Obj **resultPtr);

static int tjson_CustomConvertTypeValueToTyped(Tcl_Interp *interp, Tcl_Obj *typePtr, Tcl_Obj *valuePtr, Tcl_Obj **resultPtr) {
    int type_length;
    const char *type = Tcl_GetStringFromObj(typePtr, &type_length);
    switch (type[0]) {
        case 's':
            if (type_length == 6 && 0 == strcmp("string", type)) {
                Tcl_Obj *subListPtr = Tcl_NewListObj(0, NULL);
                Tcl_ListObjAppendElement(interp, subListPtr, Tcl_NewStringObj("S", -1));
                Tcl_ListObjAppendElement(interp, subListPtr, Tcl_DuplicateObj(valuePtr));
                *resultPtr = subListPtr;
            } else {
                Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid type", -1));
                return TCL_ERROR;
            }
            break;
        case 'i':
            if ((type_length == 3 && 0 == strcmp("int", type))
                || (type_length == 5 && 0 == strcmp("int32", type))
                || (type_length == 5 && 0 == strcmp("int64", type))) {
                long long_value;
                if (TCL_OK != Tcl_GetLongFromObj(interp, valuePtr, &long_value)) {
                    Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid int", -1));
                    return TCL_ERROR;
                }
                Tcl_Obj *subListPtr = Tcl_NewListObj(0, NULL);
                Tcl_ListObjAppendElement(interp, subListPtr, Tcl_NewStringObj("N", -1));
                Tcl_ListObjAppendElement(interp, subListPtr, Tcl_NewLongObj(long_value));

                const char *key = type_length == 3 || type[3] == '3' ? "$numberInt" : "$numberLong";
                Tcl_Obj *dictPtr = Tcl_NewDictObj();
                Tcl_DictObjPut(interp, dictPtr, Tcl_NewStringObj(key, -1), subListPtr);

                // "resultPtr" is a list of the form {M <dict>}
                Tcl_Obj *listPtr = Tcl_NewListObj(0, NULL);
                Tcl_ListObjAppendElement(interp, listPtr, Tcl_NewStringObj("M", -1));
                Tcl_ListObjAppendElement(interp, listPtr, dictPtr);
                *resultPtr = listPtr;
            } else {
                Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid type", -1));
                return TCL_ERROR;
            }
            break;
        case 'b':
            if (type_length == 7 && 0 == strcmp("boolean", type)) {
                int flag;
                if (TCL_OK != Tcl_GetBooleanFromObj(interp, valuePtr, &flag)) {
                    Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid boolean", -1));
                    return TCL_ERROR;
                }
                Tcl_Obj *subListPtr = Tcl_NewListObj(0, NULL);
                Tcl_ListObjAppendElement(interp, subListPtr, Tcl_NewStringObj("BOOL", -1));
                Tcl_ListObjAppendElement(interp, subListPtr, Tcl_NewBooleanObj(flag));
                *resultPtr = subListPtr;
            } else {
                Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid type", -1));
                return TCL_ERROR;
            }
            break;
        case 'd':
            if ((type_length == 6 && 0 == strcmp("double", type)) || (type_length == 7 && 0 == strcmp("decimal", type))) {
                double double_value;
                if (TCL_OK != Tcl_GetDoubleFromObj(interp, valuePtr, &double_value)) {
                    Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid double", -1));
                    return TCL_ERROR;
                }
                Tcl_Obj *subListPtr = Tcl_NewListObj(0, NULL);
                Tcl_ListObjAppendElement(interp, subListPtr, Tcl_NewStringObj("N", -1));
                Tcl_ListObjAppendElement(interp, subListPtr, Tcl_NewDoubleObj(double_value));

                const char *key = type_length == 6 ? "$numberDouble" : "$numberDecimal";
                Tcl_Obj *dictPtr = Tcl_NewDictObj();
                Tcl_DictObjPut(interp, dictPtr, Tcl_NewStringObj(key, -1), subListPtr);

                // "resultPtr" is a list of the form {M <dict>}
                Tcl_Obj *listPtr = Tcl_NewListObj(0, NULL);
                Tcl_ListObjAppendElement(interp, listPtr, Tcl_NewStringObj("M", -1));
                Tcl_ListObjAppendElement(interp, listPtr, dictPtr);
                *resultPtr = listPtr;
            } else if (type_length == 4 && 0 == strcmp("date", type)) {
                long long_value;
                if (TCL_OK != Tcl_GetLongFromObj(interp, valuePtr, &long_value)) {
                    Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid long in date", -1));
                    return TCL_ERROR;
                }
                Tcl_Obj *subSubListPtr = Tcl_NewListObj(0, NULL);
                Tcl_ListObjAppendElement(interp, subSubListPtr, Tcl_NewStringObj("N", -1));
                Tcl_ListObjAppendElement(interp, subSubListPtr, Tcl_NewLongObj(long_value));

                Tcl_Obj *subDictPtr = Tcl_NewDictObj();
                Tcl_DictObjPut(interp, subDictPtr, Tcl_NewStringObj("$numberLong", -1), subSubListPtr);

                Tcl_Obj *subListPtr = Tcl_NewListObj(0, NULL);
                Tcl_ListObjAppendElement(interp, subListPtr, Tcl_NewStringObj("M", -1));
                Tcl_ListObjAppendElement(interp, subListPtr, subDictPtr);

                Tcl_Obj *dictPtr = Tcl_NewDictObj();
                Tcl_DictObjPut(interp, dictPtr, Tcl_NewStringObj("$date", -1), subListPtr);

                // "resultPtr" is a list of the form {M <dict>}
                Tcl_Obj *listPtr = Tcl_NewListObj(0, NULL);
                Tcl_ListObjAppendElement(interp, listPtr, Tcl_NewStringObj("M", -1));
                Tcl_ListObjAppendElement(interp, listPtr, dictPtr);
                *resultPtr = listPtr;

            } else if (type_length == 8 && 0 == strcmp("document", type)) {

                Tcl_Obj *convertedPtr;
                if (TCL_OK != tjson_CustomToTyped(interp, valuePtr, &convertedPtr)) {
                    return TCL_ERROR;
                }
                *resultPtr = convertedPtr;
            } else {
                Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid type", -1));
                return TCL_ERROR;
            }
            break;
        case 'a':
            if (type_length == 5 && 0 == strcmp("array", type)) {
                int arrObjc;
                Tcl_Obj **arrObjv;
                if (TCL_OK != Tcl_ListObjGetElements(interp, valuePtr, &arrObjc, &arrObjv) || (arrObjc % 3 != 0)) {
                    Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid array triple notation spec", -1));
                    return TCL_ERROR;
                }
                Tcl_Obj *elemListPtr = Tcl_NewListObj(0, NULL);
                for (int j = 0; j < arrObjc; j+=3) {
                    Tcl_Obj *elemNamePtr = arrObjv[j];
                    Tcl_Obj *elemTypePtr = arrObjv[j+1];
                    Tcl_Obj *elemValuePtr = arrObjv[j+2];

                    int elemName_index;
                    if (TCL_OK != Tcl_GetIntFromObj(interp, elemNamePtr, &elemName_index) || (elemName_index != j/3)) {
                        Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid array index", -1));
                        return TCL_ERROR;
                    }

                    Tcl_Obj *convertedPtr;
                    if (TCL_OK != tjson_CustomConvertTypeValueToTyped(interp, elemTypePtr, elemValuePtr, &convertedPtr)) {
                        return TCL_ERROR;
                    }

                    Tcl_ListObjAppendElement(interp, elemListPtr, convertedPtr);

                }
                Tcl_Obj *subListPtr = Tcl_NewListObj(0, NULL);
                Tcl_ListObjAppendElement(interp, subListPtr, Tcl_NewStringObj("L", -1));
                Tcl_ListObjAppendElement(interp, subListPtr, elemListPtr);
                *resultPtr = subListPtr;
            } else {
                Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid type", -1));
                return TCL_ERROR;
            }
            break;
        case 't':
            if (type_length == 9 && 0 == strcmp("timestamp", type)) {
                // check that "valuePtr" is a list of two elements
                int value_length;
                if (TCL_OK != Tcl_ListObjLength(interp, valuePtr, &value_length) || value_length != 2) {
                    Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid timestamp", -1));
                    return TCL_ERROR;
                }

                // the first element is "timestamp" and the second is "increment"
                Tcl_Obj *timestampPtr;
                Tcl_Obj *incrementPtr;
                if (TCL_OK != Tcl_ListObjIndex(interp, valuePtr, 0, &timestampPtr)
                    || TCL_OK != Tcl_ListObjIndex(interp, valuePtr, 1, &incrementPtr)) {
                    Tcl_SetObjResult(interp, Tcl_NewStringObj("error while extracting pattern and options from timestamp", -1));
                    return TCL_ERROR;
                }

                Tcl_Obj *patternSubSubListPtr = Tcl_NewListObj(0, NULL);
                Tcl_ListObjAppendElement(interp, patternSubSubListPtr, Tcl_NewStringObj("S", -1));
                Tcl_ListObjAppendElement(interp, patternSubSubListPtr, Tcl_DuplicateObj(timestampPtr));

                Tcl_Obj *optionsSubSubListPtr = Tcl_NewListObj(0, NULL);
                Tcl_ListObjAppendElement(interp, optionsSubSubListPtr, Tcl_NewStringObj("S", -1));
                Tcl_ListObjAppendElement(interp, optionsSubSubListPtr, Tcl_DuplicateObj(incrementPtr));

                Tcl_Obj *subDictPtr = Tcl_NewDictObj();
                Tcl_DictObjPut(interp, subDictPtr, Tcl_NewStringObj("t", -1), patternSubSubListPtr);
                Tcl_DictObjPut(interp, subDictPtr, Tcl_NewStringObj("i", -1), optionsSubSubListPtr);

                Tcl_Obj *subListPtr = Tcl_NewListObj(0, NULL);
                Tcl_ListObjAppendElement(interp, subListPtr, Tcl_NewStringObj("M", -1));
                Tcl_ListObjAppendElement(interp, subListPtr, subDictPtr);

                Tcl_Obj *dictPtr = Tcl_NewDictObj();
                Tcl_DictObjPut(interp, dictPtr, Tcl_NewStringObj("$timestamp", -1), subListPtr);

                // "resultPtr" is a list of the form {M <dict>}
                Tcl_Obj *listPtr = Tcl_NewListObj(0, NULL);
                Tcl_ListObjAppendElement(interp, listPtr, Tcl_NewStringObj("M", -1));
                Tcl_ListObjAppendElement(interp, listPtr, dictPtr);
                *resultPtr = listPtr;
            } else {
                Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid type", -1));
                return TCL_ERROR;
            }
            break;
        case 'r':
            if (type_length == 5 && 0 == strcmp("regex", type)) {
                // check that "valuePtr" is a list of two elements
                int value_length;
                if (TCL_OK != Tcl_ListObjLength(interp, valuePtr, &value_length) || value_length != 2) {
                    Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid regex", -1));
                    return TCL_ERROR;
                }

                // the first element is "pattern" and the second is "options"
                Tcl_Obj *patternPtr;
                Tcl_Obj *optionsPtr;
                if (TCL_OK != Tcl_ListObjIndex(interp, valuePtr, 0, &patternPtr)
                    || TCL_OK != Tcl_ListObjIndex(interp, valuePtr, 1, &optionsPtr)) {
                    Tcl_SetObjResult(interp, Tcl_NewStringObj("error while extracting pattern and options from regex", -1));
                    return TCL_ERROR;
                }

                Tcl_Obj *patternSubSubListPtr = Tcl_NewListObj(0, NULL);
                Tcl_ListObjAppendElement(interp, patternSubSubListPtr, Tcl_NewStringObj("S", -1));
                Tcl_ListObjAppendElement(interp, patternSubSubListPtr, Tcl_DuplicateObj(patternPtr));

                Tcl_Obj *optionsSubSubListPtr = Tcl_NewListObj(0, NULL);
                Tcl_ListObjAppendElement(interp, optionsSubSubListPtr, Tcl_NewStringObj("S", -1));
                Tcl_ListObjAppendElement(interp, optionsSubSubListPtr, Tcl_DuplicateObj(optionsPtr));

                Tcl_Obj *subDictPtr = Tcl_NewDictObj();
                Tcl_DictObjPut(interp, subDictPtr, Tcl_NewStringObj("pattern", -1), patternSubSubListPtr);
                Tcl_DictObjPut(interp, subDictPtr, Tcl_NewStringObj("options", -1), optionsSubSubListPtr);

                Tcl_Obj *subListPtr = Tcl_NewListObj(0, NULL);
                Tcl_ListObjAppendElement(interp, subListPtr, Tcl_NewStringObj("M", -1));
                Tcl_ListObjAppendElement(interp, subListPtr, subDictPtr);

                Tcl_Obj *dictPtr = Tcl_NewDictObj();
                Tcl_DictObjPut(interp, dictPtr, Tcl_NewStringObj("$regularExpression", -1), subListPtr);

                // "resultPtr" is a list of the form {M <dict>}
                Tcl_Obj *listPtr = Tcl_NewListObj(0, NULL);
                Tcl_ListObjAppendElement(interp, listPtr, Tcl_NewStringObj("M", -1));
                Tcl_ListObjAppendElement(interp, listPtr, dictPtr);
                *resultPtr = listPtr;
            } else {
                Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid type", -1));
                return TCL_ERROR;
            }
            break;
        case 'o':
            if (type_length == 3 && 0 == strcmp("oid", type)) {
                // check that "valuePtr" is the oid

                Tcl_Obj *subListPtr = Tcl_NewListObj(0, NULL);
                Tcl_ListObjAppendElement(interp, subListPtr, Tcl_NewStringObj("S", -1));
                Tcl_ListObjAppendElement(interp, subListPtr, Tcl_DuplicateObj(valuePtr));

                Tcl_Obj *dictPtr = Tcl_NewDictObj();
                Tcl_DictObjPut(interp, dictPtr, Tcl_NewStringObj("$oid", -1), subListPtr);

                // "resultPtr" is a list of the form {M <dict>}
                Tcl_Obj *listPtr = Tcl_NewListObj(0, NULL);
                Tcl_ListObjAppendElement(interp, listPtr, Tcl_NewStringObj("M", -1));
                Tcl_ListObjAppendElement(interp, listPtr, dictPtr);
                *resultPtr = listPtr;
            } else {
                Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid type", -1));
                return TCL_ERROR;
            }
            break;
        default:
            Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid type in spec in custom convert type-value to typed", -1));
            return TCL_ERROR;  /* invalid type */
    }

    return TCL_OK;
}

int tjson_CustomToTyped(Tcl_Interp *interp, Tcl_Obj *specPtr, Tcl_Obj **resultPtr) {

    int objc;
    Tcl_Obj **objv;
    if (TCL_OK != Tcl_ListObjGetElements(interp, specPtr, &objc, &objv) || (objc % 3 != 0)) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid triple notation spec", -1));
        return TCL_ERROR;  /* invalid spec length */
    }

    Tcl_Obj *dictPtr = Tcl_NewDictObj();
    for (int i = 0; i < objc; i+=3) {
        Tcl_Obj *namePtr = objv[i];
        Tcl_Obj *typePtr = objv[i+1];
        Tcl_Obj *valuePtr = objv[i+2];

        Tcl_Obj *convertedPtr;
        if (TCL_OK != tjson_CustomConvertTypeValueToTyped(interp, typePtr, valuePtr, &convertedPtr)) {
            return TCL_ERROR;
        }
        Tcl_DictObjPut(interp, dictPtr, namePtr, convertedPtr);
    }
    Tcl_Obj *listPtr = Tcl_NewListObj(0, NULL);
    Tcl_ListObjAppendElement(interp, listPtr, Tcl_NewStringObj("M", -1));
    Tcl_ListObjAppendElement(interp, listPtr, dictPtr);
    *resultPtr = listPtr;
    return TCL_OK;
}

static int tjson_TypedConvertTimestampToCustom(Tcl_Interp *interp, Tcl_Obj *specPtr, Tcl_Obj **resultPtr) {
    // "specPtr" is a list of two elements, the first of which is "M" and the second is a dict of two keys,
    //      "t" for timestamp and "i" for increment
    // "t" is a list of two elements, the first of which is "N" and the second is the timestamp
    // "i" is a list of two elements, the first of which is "N" and the second is the increment

    // check if "specPtr" is a list of two elements
    int spec_length;
    if (TCL_OK != Tcl_ListObjLength(interp, specPtr, &spec_length) || spec_length != 2) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid typed spec, spec length != 2", -1));
        return TCL_ERROR;
    }

    // check if "specPtr" is of the form {M <dict>}
    Tcl_Obj *typePtr;
    Tcl_Obj *valuePtr;
    if (TCL_OK != Tcl_ListObjIndex(interp, specPtr, 0, &typePtr)
        || TCL_OK != Tcl_ListObjIndex(interp, specPtr, 1, &valuePtr)) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("error while extracting type and value from typed", -1));
        return TCL_ERROR;
    }
    int type_length;
    const char *type = Tcl_GetStringFromObj(typePtr, &type_length);
    if (type_length != 1 || type[0] != 'M') {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid typed spec, type != M inside $timestamp", -1));
        return TCL_ERROR;
    }

    Tcl_Obj *tTypedPtr;
    Tcl_Obj *iTypedPtr;
    int dict_size;
    if (TCL_OK == Tcl_DictObjSize(interp, valuePtr, &dict_size)
        && dict_size == 2
        && TCL_OK == Tcl_DictObjGet(interp, valuePtr, Tcl_NewStringObj("t", -1), &tTypedPtr)
        && TCL_OK == Tcl_DictObjGet(interp, valuePtr, Tcl_NewStringObj("i", -1), &iTypedPtr)) {

        // check if "tTypedPtr" is a list of two elements
        int tTyped_length;
        if (TCL_OK != Tcl_ListObjLength(interp, tTypedPtr, &tTyped_length) || tTyped_length != 2) {
            Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid typed spec, tTyped length != 2", -1));
            return TCL_ERROR;
        }

        // check if "iTypedPtr" is a list of two elements
        int iTyped_length;
        if (TCL_OK != Tcl_ListObjLength(interp, iTypedPtr, &iTyped_length) || iTyped_length != 2) {
            Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid typed spec, iTyped length != 2", -1));
            return TCL_ERROR;
        }

        // check the types that "tTypedPtr" and "iTypedPtr" are of the form {N <value>}
        Tcl_Obj *tTypedTypePtr;
        Tcl_Obj *iTypedTypePtr;
        if (TCL_OK != Tcl_ListObjIndex(interp, tTypedPtr, 0, &tTypedTypePtr)
            || TCL_OK != Tcl_ListObjIndex(interp, iTypedPtr, 0, &iTypedTypePtr)) {
            Tcl_SetObjResult(interp, Tcl_NewStringObj("error while extracting t and i types from typed", -1));
            return TCL_ERROR;
        }

        int tTypedType_length;
        const char *tTypedType = Tcl_GetStringFromObj(tTypedTypePtr, &tTypedType_length);
        int iTypedType_length;
        const char *iTypedType = Tcl_GetStringFromObj(iTypedTypePtr, &iTypedType_length);
        if (tTypedType_length != 1 || iTypedType_length != 1
            || tTypedType[0] != 'N' || iTypedType[0] != 'N') {
            Tcl_SetObjResult(interp,
                             Tcl_NewStringObj("invalid typed spec, tTyped or iTyped not of the form {N <value>}", -1));
            return TCL_ERROR;
        }

        Tcl_Obj *tValuePtr;
        Tcl_Obj *iValuePtr;
        if (TCL_OK != Tcl_ListObjIndex(interp, tTypedPtr, 1, &tValuePtr)
            || TCL_OK != Tcl_ListObjIndex(interp, iTypedPtr, 1, &iValuePtr)) {
            Tcl_SetObjResult(interp, Tcl_NewStringObj("error while extracting t and i from typed", -1));
            return TCL_ERROR;
        }

        // add "tPtr" and "iPtr" to "subListPtr"
        Tcl_Obj *subListPtr = Tcl_NewListObj(0, NULL);
        Tcl_ListObjAppendElement(interp, subListPtr, tValuePtr);
        Tcl_ListObjAppendElement(interp, subListPtr, iValuePtr);

        // create the final output of type "timestamp" with "subListPtr" as value
        Tcl_Obj *listPtr = Tcl_NewListObj(0, NULL);
        Tcl_ListObjAppendElement(interp, listPtr, Tcl_NewStringObj("timestamp", -1));
        Tcl_ListObjAppendElement(interp, listPtr, subListPtr);
        *resultPtr = listPtr;
        return TCL_OK;
    } else {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid typed spec, not of the form {M <dict>} inside $timestamp", -1));
        return TCL_ERROR;
    }
}

static int tjson_TypedConvertRegularExpressionToCustom(Tcl_Interp *interp, Tcl_Obj *specPtr, Tcl_Obj **resultPtr) {
    // "specPtr" is a list of two elements, the first of which is "M" and the second is a dict of two keys,
    //      "pattern" for timestamp and "options" for increment
    // "pattern" is a list of two elements, the first of which is "S" and the second is the pattern
    // "options" is a list of two elements, the first of which is "S" and the second is the options
    // check if "specPtr" is a list of two elements
    int spec_length;
    if (TCL_OK != Tcl_ListObjLength(interp, specPtr, &spec_length) || spec_length != 2) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid typed spec, spec length != 2", -1));
        return TCL_ERROR;
    }

    // check if "specPtr" is of the form {M <dict>}
    Tcl_Obj *typePtr;
    Tcl_Obj *valuePtr;
    if (TCL_OK != Tcl_ListObjIndex(interp, specPtr, 0, &typePtr)
        || TCL_OK != Tcl_ListObjIndex(interp, specPtr, 1, &valuePtr)) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("error while extracting type and value from typed", -1));
        return TCL_ERROR;
    }
    int type_length;
    const char *type = Tcl_GetStringFromObj(typePtr, &type_length);
    if (type_length != 1 || type[0] != 'M') {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid typed spec, type != M inside $regularExpression", -1));
        return TCL_ERROR;
    }

    Tcl_Obj *patternTypedPtr;
    Tcl_Obj *optionsTypedPtr;
    int dict_size;
    if (TCL_OK == Tcl_DictObjSize(interp, valuePtr, &dict_size)
        && dict_size == 2
        && TCL_OK == Tcl_DictObjGet(interp, valuePtr, Tcl_NewStringObj("pattern", -1), &patternTypedPtr)
        && TCL_OK == Tcl_DictObjGet(interp, valuePtr, Tcl_NewStringObj("options", -1), &optionsTypedPtr)) {

        // check if "patternTypedPtr" is a list of two elements
        int patternTyped_length;
        if (TCL_OK != Tcl_ListObjLength(interp, patternTypedPtr, &patternTyped_length) || patternTyped_length != 2) {
            Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid typed spec, patternTyped length != 2", -1));
            return TCL_ERROR;
        }

        // check if "optionsTypedPtr" is a list of two elements
        int optionsTyped_length;
        if (TCL_OK != Tcl_ListObjLength(interp, optionsTypedPtr, &optionsTyped_length) || optionsTyped_length != 2) {
            Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid typed spec, optionsTyped length != 2", -1));
            return TCL_ERROR;
        }

        // check the types that "patternTypedPtr" and "optionsTypedPtr" are of the form {N <value>}
        Tcl_Obj *patternTypedTypePtr;
        Tcl_Obj *optionsTypedTypePtr;
        if (TCL_OK != Tcl_ListObjIndex(interp, patternTypedPtr, 0, &patternTypedTypePtr)
            || TCL_OK != Tcl_ListObjIndex(interp, optionsTypedPtr, 0, &optionsTypedTypePtr)) {
            Tcl_SetObjResult(interp, Tcl_NewStringObj("error while extracting pattern and options types from typed", -1));
            return TCL_ERROR;
        }

        int patternTypedType_length;
        const char *patternTypedType = Tcl_GetStringFromObj(patternTypedTypePtr, &patternTypedType_length);
        int optionsTypedType_length;
        const char *optionsTypedType = Tcl_GetStringFromObj(optionsTypedTypePtr, &optionsTypedType_length);
        if (patternTypedType_length != 1 || optionsTypedType_length != 1
            || patternTypedType[0] != 'S' || optionsTypedType[0] != 'S') {
            Tcl_SetObjResult(interp,
                             Tcl_NewStringObj("invalid typed spec, patternTyped or optionsTyped not of the form {N <value>}", -1));
            return TCL_ERROR;
        }

        Tcl_Obj *patternValuePtr;
        Tcl_Obj *optionsValuePtr;
        if (TCL_OK != Tcl_ListObjIndex(interp, patternTypedPtr, 1, &patternValuePtr)
            || TCL_OK != Tcl_ListObjIndex(interp, optionsTypedPtr, 1, &optionsValuePtr)) {
            Tcl_SetObjResult(interp, Tcl_NewStringObj("error while extracting pattern and options from typed", -1));
            return TCL_ERROR;
        }

        // add "tPtr" and "iPtr" to "subListPtr"
        Tcl_Obj *subListPtr = Tcl_NewListObj(0, NULL);
        Tcl_ListObjAppendElement(interp, subListPtr, patternValuePtr);
        Tcl_ListObjAppendElement(interp, subListPtr, optionsValuePtr);

        // create the final output of type "timestamp" with "subListPtr" as value
        Tcl_Obj *listPtr = Tcl_NewListObj(0, NULL);
        Tcl_ListObjAppendElement(interp, listPtr, Tcl_NewStringObj("regex", -1));
        Tcl_ListObjAppendElement(interp, listPtr, subListPtr);
        *resultPtr = listPtr;
        return TCL_OK;
    } else {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid typed spec, not of the form {M <dict>} inside $regularExpression", -1));
        return TCL_ERROR;
    }

}

static int tjson_TypedConvertNumberIntToCustom(Tcl_Interp *interp, Tcl_Obj *specPtr, Tcl_Obj **resultPtr) {
    // check if "specPtr" is a list of two elements
    int spec_length;
    if (TCL_OK != Tcl_ListObjLength(interp, specPtr, &spec_length) || spec_length != 2) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid typed spec, spec length != 2", -1));
        return TCL_ERROR;
    }

    // check if "specPtr" is of the form {M <dict>}
    Tcl_Obj *typePtr;
    Tcl_Obj *valuePtr;
    if (TCL_OK != Tcl_ListObjIndex(interp, specPtr, 0, &typePtr)
        || TCL_OK != Tcl_ListObjIndex(interp, specPtr, 1, &valuePtr)) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("error while extracting type and value from typed", -1));
        return TCL_ERROR;
    }
    int type_length;
    const char *type = Tcl_GetStringFromObj(typePtr, &type_length);
    if (type_length != 1 || type[0] != 'N') {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid typed spec, type != N inside $numberInt", -1));
        return TCL_ERROR;
    }

    // try to read integer from "valuePtr"
    int int_value;
    if (TCL_OK != Tcl_GetIntFromObj(interp, valuePtr, &int_value)) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid int", -1));
        return TCL_ERROR;
    }

    // "resultPtr" is a list of the form {N <value>}
    Tcl_Obj *listPtr = Tcl_NewListObj(0, NULL);
    Tcl_ListObjAppendElement(interp, listPtr, Tcl_NewStringObj("int32", -1));
    Tcl_ListObjAppendElement(interp, listPtr, Tcl_NewIntObj(int_value));
    *resultPtr = listPtr;
    return TCL_OK;
}

static int tjson_TypedConvertNumberLongToCustom(Tcl_Interp *interp, Tcl_Obj *specPtr, Tcl_Obj **resultPtr) {
    // check if "specPtr" is a list of two elements
    int spec_length;
    if (TCL_OK != Tcl_ListObjLength(interp, specPtr, &spec_length) || spec_length != 2) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid typed spec, spec length != 2", -1));
        return TCL_ERROR;
    }

    // check if "specPtr" is of the form {M <dict>}
    Tcl_Obj *typePtr;
    Tcl_Obj *valuePtr;
    if (TCL_OK != Tcl_ListObjIndex(interp, specPtr, 0, &typePtr)
        || TCL_OK != Tcl_ListObjIndex(interp, specPtr, 1, &valuePtr)) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("error while extracting type and value from typed", -1));
        return TCL_ERROR;
    }
    int type_length;
    const char *type = Tcl_GetStringFromObj(typePtr, &type_length);
    if (type_length != 1 || type[0] != 'N') {
        fprintf(stderr, "type: %s\n", type);
        Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid typed spec, type != N inside $numberLong", -1));
        return TCL_ERROR;
    }

    // try to read integer from "valuePtr"
    long long_value;
    if (TCL_OK != Tcl_GetLongFromObj(interp, valuePtr, &long_value)) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid long", -1));
        return TCL_ERROR;
    }

    // "resultPtr" is a list of the form {N <value>}
    Tcl_Obj *listPtr = Tcl_NewListObj(0, NULL);
    Tcl_ListObjAppendElement(interp, listPtr, Tcl_NewStringObj("int64", -1));
    Tcl_ListObjAppendElement(interp, listPtr, Tcl_NewLongObj(long_value));
    *resultPtr = listPtr;

    return TCL_OK;
}

static int tjson_TypedConvertNumberDoubleToCustom(Tcl_Interp *interp, Tcl_Obj *specPtr, Tcl_Obj **resultPtr) {
    // check if "specPtr" is a list of two elements
    int spec_length;
    if (TCL_OK != Tcl_ListObjLength(interp, specPtr, &spec_length) || spec_length != 2) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid typed spec, spec length != 2", -1));
        return TCL_ERROR;
    }

    // check if "specPtr" is of the form {M <dict>}
    Tcl_Obj *typePtr;
    Tcl_Obj *valuePtr;
    if (TCL_OK != Tcl_ListObjIndex(interp, specPtr, 0, &typePtr)
        || TCL_OK != Tcl_ListObjIndex(interp, specPtr, 1, &valuePtr)) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("error while extracting type and value from typed", -1));
        return TCL_ERROR;
    }
    int type_length;
    const char *type = Tcl_GetStringFromObj(typePtr, &type_length);
    if (type_length != 1 || type[0] != 'N') {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid typed spec, type != N inside $numberDouble", -1));
        return TCL_ERROR;
    }

    // try to read integer from "valuePtr"
    double double_value;
    if (TCL_OK != Tcl_GetDoubleFromObj(interp, valuePtr, &double_value)) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid double", -1));
        return TCL_ERROR;
    }

    // "resultPtr" is a list of the form {N <value>}
    Tcl_Obj *listPtr = Tcl_NewListObj(0, NULL);
    Tcl_ListObjAppendElement(interp, listPtr, Tcl_NewStringObj("double", -1));
    Tcl_ListObjAppendElement(interp, listPtr, Tcl_NewDoubleObj(double_value));
    *resultPtr = listPtr;
    return TCL_OK;
}

static int tjson_TypedConvertNumberDecimalToCustom(Tcl_Interp *interp, Tcl_Obj *specPtr, Tcl_Obj **resultPtr) {
    return tjson_TypedConvertNumberDoubleToCustom(interp, specPtr, resultPtr);
}

static int tjson_TypedConvertMinKeyToCustom(Tcl_Interp *interp, Tcl_Obj *specPtr, Tcl_Obj **resultPtr) {
    return tjson_TypedConvertNumberLongToCustom(interp, specPtr, resultPtr);
}

static int tjson_TypedConvertMaxKeyToCustom(Tcl_Interp *interp, Tcl_Obj *specPtr, Tcl_Obj **resultPtr) {
    return tjson_TypedConvertNumberLongToCustom(interp, specPtr, resultPtr);
}

static int tjson_TypedConvertOidToCustom(Tcl_Interp *interp, Tcl_Obj *specPtr, Tcl_Obj **resultPtr) {
    // "specPtr" must be a dict of the form {M <dict>}
    // "dict" must have a key "$oid" with a value of the form {S <value>}
    // "value" must be a string


    // check if "specPtr" is a list of two elements
    int spec_length;
    if (TCL_OK != Tcl_ListObjLength(interp, specPtr, &spec_length) || spec_length != 2) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid typed spec, spec length != 2", -1));
        return TCL_ERROR;
    }

    // check if "specPtr" is of the form {M <dict>}
    Tcl_Obj *typePtr;
    Tcl_Obj *valuePtr;
    if (TCL_OK != Tcl_ListObjIndex(interp, specPtr, 0, &typePtr)
        || TCL_OK != Tcl_ListObjIndex(interp, specPtr, 1, &valuePtr)) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("error while extracting type and value from typed", -1));
        return TCL_ERROR;
    }

    int type_length;
    const char *type = Tcl_GetStringFromObj(typePtr, &type_length);
    if (type_length != 1 || type[0] != 'S') {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid typed spec, type != M inside $oid", -1));
        return TCL_ERROR;
    }

    // try to read string from "oidValueValuePtr"
    int value_length;
    const char *value = Tcl_GetStringFromObj(valuePtr, &value_length);

    // "resultPtr" is a list of the form {N <value>}
    Tcl_Obj *listPtr = Tcl_NewListObj(0, NULL);
    Tcl_ListObjAppendElement(interp, listPtr, Tcl_NewStringObj("oid", -1));
    Tcl_ListObjAppendElement(interp, listPtr, Tcl_NewStringObj(value, value_length));
    *resultPtr = listPtr;
    return TCL_OK;

}

static int tjson_TypedConvertDateToCustom(Tcl_Interp *interp, Tcl_Obj *specPtr, Tcl_Obj **resultPtr) {
    // "specPtr" must be a dict of the form {M <dict>}
    // "dict" must have a key "$numberLong" with a value of the form {N <value>}
    // "value" must be a long

    // check if "specPtr" is a list of two elements
    int spec_length;
    if (TCL_OK != Tcl_ListObjLength(interp, specPtr, &spec_length) || spec_length != 2) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid typed spec, spec length != 2", -1));
        return TCL_ERROR;
    }

    // check if "specPtr" is of the form {M <dict>}
    Tcl_Obj *typePtr;
    Tcl_Obj *valuePtr;
    if (TCL_OK != Tcl_ListObjIndex(interp, specPtr, 0, &typePtr)
        || TCL_OK != Tcl_ListObjIndex(interp, specPtr, 1, &valuePtr)) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("error while extracting type and value from typed", -1));
        return TCL_ERROR;
    }

    int type_length;
    const char *type = Tcl_GetStringFromObj(typePtr, &type_length);
    if (type_length != 1 || type[0] != 'M') {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid typed spec, type != M inside $date", -1));
        return TCL_ERROR;
    }

    // check if "valuePtr" is a dict
    int dict_size;
    if (TCL_OK != Tcl_DictObjSize(interp, valuePtr, &dict_size) || dict_size != 1) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid typed spec, value not a dict", -1));
        return TCL_ERROR;
    }

    // check if "valuePtr" is of the form {$numberLong <value>}
    Tcl_Obj *numberLongValuePtr;
    if (TCL_OK != Tcl_DictObjGet(interp, valuePtr, Tcl_NewStringObj("$numberLong", -1), &numberLongValuePtr)) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid typed spec, value not of the form {$numberLong <value>}", -1));
        return TCL_ERROR;
    }

    // check if "numberLongValuePtr" is of the form {N <value>}
    Tcl_Obj *numberLongTypePtr;
    Tcl_Obj *numberLongValueValuePtr;
    if (TCL_OK != Tcl_ListObjIndex(interp, numberLongValuePtr, 0, &numberLongTypePtr)
        || TCL_OK != Tcl_ListObjIndex(interp, numberLongValuePtr, 1, &numberLongValueValuePtr)) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("error while extracting type and value from $date", -1));
        return TCL_ERROR;
    }

    int numberLongType_length;
    const char *numberLongType = Tcl_GetStringFromObj(numberLongTypePtr, &numberLongType_length);
    if (numberLongType_length != 1 || numberLongType[0] != 'N') {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid typed spec, type != N inside $date", -1));
        return TCL_ERROR;
    }

    // try to read long integer from "numberLongValueValuePtr"
    long long_value;
    if (TCL_OK != Tcl_GetLongFromObj(interp, numberLongValueValuePtr, &long_value)) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid long", -1));
        return TCL_ERROR;
    }

    // "resultPtr" is a list of the form {N <value>}
    Tcl_Obj *listPtr = Tcl_NewListObj(0, NULL);
    Tcl_ListObjAppendElement(interp, listPtr, Tcl_NewStringObj("date", -1));
    Tcl_ListObjAppendElement(interp, listPtr, Tcl_NewLongObj(long_value));
    *resultPtr = listPtr;
    return TCL_OK;
}

static int tjson_TypedConvertTypeValueToCustom(Tcl_Interp *interp, Tcl_Obj *specPtr, Tcl_Obj **resultPtr) {
    // "specPtr" must be a list of two elements, the first of which must be the "type" and the second the "value"
    int length;
    if (TCL_OK != Tcl_ListObjLength(interp, specPtr, &length) || length != 2) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid typed spec, must be a list of two elements", -1));
        return TCL_ERROR;
    }

    Tcl_Obj *typePtr;
    Tcl_Obj *valuePtr;
    if (TCL_OK != Tcl_ListObjIndex(interp, specPtr, 0, &typePtr)
        || TCL_OK != Tcl_ListObjIndex(interp, specPtr, 1, &valuePtr)) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("error while extracting type and value from list", -1));
        return TCL_ERROR;
    }

    int type_length;
    const char *type = Tcl_GetStringFromObj(typePtr, &type_length);
    switch(type[0]) {
        case 'S':
            if (type_length == 1) {
                Tcl_Obj *subListPtr = Tcl_NewListObj(0, NULL);
                Tcl_ListObjAppendElement(interp, subListPtr, Tcl_NewStringObj("string", -1));
                Tcl_ListObjAppendElement(interp, subListPtr, Tcl_DuplicateObj(valuePtr));
                *resultPtr = subListPtr;
            } else {
                Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid type", -1));
                return TCL_ERROR;
            }
            break;
        case 'N':
            if (type_length == 1) {
                int int_value;
                Tcl_Obj *numTypePtr = NULL;
                Tcl_Obj *numValuePtr = NULL;
                if (TCL_OK != Tcl_GetIntFromObj(interp, valuePtr, &int_value)) {
                    long long_value;
                    if (TCL_OK != Tcl_GetLongFromObj(interp, valuePtr, &long_value)) {
                        double double_value;
                        if (TCL_OK != Tcl_GetDoubleFromObj(interp, valuePtr, &double_value)) {
                            Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid double", -1));
                            return TCL_ERROR;
                        } else {
                            numTypePtr = Tcl_NewStringObj("double", -1);
                            numValuePtr = Tcl_NewDoubleObj(double_value);
                        }
                    } else {
                        numTypePtr = Tcl_NewStringObj("int64", -1);
                        numValuePtr = Tcl_NewLongObj(long_value);
                    }
                } else {
                    numTypePtr = Tcl_NewStringObj("int32", -1);
                    numValuePtr = Tcl_NewIntObj(int_value);
                }
                Tcl_Obj *listPtr = Tcl_NewListObj(0, NULL);
                Tcl_ListObjAppendElement(interp, listPtr, numTypePtr);
                Tcl_ListObjAppendElement(interp, listPtr, numValuePtr);
                *resultPtr = listPtr;
            } else {
                Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid type", -1));
                return TCL_ERROR;
            }
            break;
        case 'B':
            if (type_length == 4 && 0 == strcmp("BOOL", type)) {
                int flag;
                if (TCL_OK != Tcl_GetBooleanFromObj(interp, valuePtr, &flag)) {
                    Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid boolean", -1));
                    return TCL_ERROR;
                }
                Tcl_Obj *subListPtr = Tcl_NewListObj(0, NULL);
                Tcl_ListObjAppendElement(interp, subListPtr, Tcl_NewStringObj("boolean", -1));
                Tcl_ListObjAppendElement(interp, subListPtr, Tcl_NewBooleanObj(flag));
                *resultPtr = subListPtr;
            } else {
                Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid type", -1));
                return TCL_ERROR;
            }
            break;
        case 'L':
            if (type_length == 1) {
                int arrObjc;
                Tcl_Obj **arrObjv;
                if (TCL_OK != Tcl_ListObjGetElements(interp, valuePtr, &arrObjc, &arrObjv)) {
                    Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid array triple notation spec", -1));
                    return TCL_ERROR;
                }
                Tcl_Obj *elemListPtr = Tcl_NewListObj(0, NULL);
                for (int j = 0; j < arrObjc; j++) {
                    Tcl_Obj *elemValuePtr = arrObjv[j];

                    Tcl_Obj *convertedPtr;
                    if (TCL_OK != tjson_TypedConvertTypeValueToCustom(interp, elemValuePtr, &convertedPtr)) {
                        return TCL_ERROR;
                    }

                    Tcl_Obj *convertedTypePtr;
                    Tcl_Obj *convertedValuePtr;
                    if (TCL_OK != Tcl_ListObjIndex(interp, convertedPtr, 0, &convertedTypePtr)
                        || TCL_OK != Tcl_ListObjIndex(interp, convertedPtr, 1, &convertedValuePtr)) {
                        Tcl_SetObjResult(interp, Tcl_NewStringObj("error while extracting type and value from converted", -1));
                        return TCL_ERROR;
                    }

                    Tcl_ListObjAppendElement(interp, elemListPtr, Tcl_NewIntObj(j));
                    Tcl_ListObjAppendElement(interp, elemListPtr, convertedTypePtr);
                    Tcl_ListObjAppendElement(interp, elemListPtr, convertedValuePtr);
                }
                Tcl_Obj *listPtr = Tcl_NewListObj(0, NULL);
                Tcl_ListObjAppendElement(interp, listPtr, Tcl_NewStringObj("array", -1));
                Tcl_ListObjAppendElement(interp, listPtr, elemListPtr);
                *resultPtr = listPtr;
            } else {
                Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid type", -1));
                return TCL_ERROR;
            }
            break;
        case 'M':
            if (type_length == 1) {
                // check if the typed structure is a dict of one item:
                //      "$timestamp" -> {M <dict>}
                //      "$numberInt" -> {N <value>}
                //      "$numberLong" -> {N <value>}
                //      "$numberDouble" -> {N <value>}
                //      "$numberDecimal" -> {N <value>}
                //      "$minKey" -> {N <value>}
                //      "$maxKey" -> {N <value>}
                //      "$date -> {M <dict>}
                //      "$regularExpression -> {M <dict>}

                int dict_size;
                if (TCL_OK == Tcl_DictObjSize(interp, valuePtr, &dict_size) && dict_size == 1) {
                    Tcl_DictSearch search;
                    Tcl_Obj *dictKeyPtr;
                    Tcl_Obj *dictValuePtr;
                    int done;
                    if (TCL_OK != Tcl_DictObjFirst(interp, valuePtr, &search, &dictKeyPtr, &dictValuePtr, &done)) {
                        Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid typed spec, first from dict", -1));
                        return TCL_ERROR;
                    }
                    int dict_key_length;
                    const char *dict_key = Tcl_GetStringFromObj(dictKeyPtr, &dict_key_length);
                    if (dict_key_length == 10 && 0 == strncmp("$timestamp", dict_key, dict_key_length)) {
                        if (TCL_OK != tjson_TypedConvertTimestampToCustom(interp, dictValuePtr, resultPtr)) {
                            return TCL_ERROR;
                        }
                        break;
                    } else if (dict_key_length == 18 && 0 == strncmp("$regularExpression", dict_key, dict_key_length)) {
                        if (TCL_OK != tjson_TypedConvertRegularExpressionToCustom(interp, dictValuePtr, resultPtr)) {
                            return TCL_ERROR;
                        }
                        break;
                    } else if (dict_key_length == 5 && 0 == strncmp("$date", dict_key, dict_key_length)) {
                        if (TCL_OK != tjson_TypedConvertDateToCustom(interp, dictValuePtr, resultPtr)) {
                            return TCL_ERROR;
                        }
                        break;
                    } else if (dict_key_length == 10 && 0 == strncmp("$numberInt", dict_key, dict_key_length)) {
                        if (TCL_OK != tjson_TypedConvertNumberIntToCustom(interp, dictValuePtr, resultPtr)) {
                            return TCL_ERROR;
                        }
                        break;
                    } else if (dict_key_length == 11 && 0 == strncmp("$numberLong", dict_key, dict_key_length)) {
                        if (TCL_OK != tjson_TypedConvertNumberLongToCustom(interp, dictValuePtr, resultPtr)) {
                            return TCL_ERROR;
                        }
                        break;
                    } else if (dict_key_length == 13 && 0 == strncmp("$numberDouble", dict_key, dict_key_length)) {
                        if (TCL_OK != tjson_TypedConvertNumberDoubleToCustom(interp, dictValuePtr, resultPtr)) {
                            return TCL_ERROR;
                        }
                        break;
                    } else if (dict_key_length == 14 && 0 == strncmp("$numberDecimal", dict_key, dict_key_length)) {
                        if (TCL_OK != tjson_TypedConvertNumberDecimalToCustom(interp, dictValuePtr, resultPtr)) {
                            return TCL_ERROR;
                        }
                        break;
                    } else if (dict_key_length == 7 && 0 == strncmp("$minKey", dict_key, dict_key_length)) {
                        if (TCL_OK != tjson_TypedConvertMinKeyToCustom(interp, dictValuePtr, resultPtr)) {
                            return TCL_ERROR;
                        }
                        break;
                    } else if (dict_key_length == 7 && 0 == strncmp("$maxKey", dict_key, dict_key_length)) {
                        if (TCL_OK != tjson_TypedConvertMaxKeyToCustom(interp, dictValuePtr, resultPtr)) {
                            return TCL_ERROR;
                        }
                        break;
                    } else if (dict_key_length == 4 && 0 == strncmp("$oid", dict_key, dict_key_length)) {
                        if (TCL_OK != tjson_TypedConvertOidToCustom(interp, dictValuePtr, resultPtr)) {
                            return TCL_ERROR;
                        }
                        break;
                    }
                }

                // "valuePtr" is a dict
                Tcl_DictSearch search;
                Tcl_Obj *dictKeyPtr;
                Tcl_Obj *dictValuePtr;
                int done;
                if (TCL_OK != Tcl_DictObjFirst(interp, valuePtr, &search, &dictKeyPtr, &dictValuePtr, &done)) {
                    Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid typed spec, first from dict", -1));
                    return TCL_ERROR;
                }
                Tcl_Obj *subListPtr = Tcl_NewListObj(0, NULL);
                for (; !done; Tcl_DictObjNext(&search, &dictKeyPtr, &dictValuePtr, &done)) {
                    Tcl_Obj *convertedPtr;
                    if (TCL_OK != tjson_TypedConvertTypeValueToCustom(interp, dictValuePtr, &convertedPtr)) {
                        return TCL_ERROR;
                    }

                    // extract "convertedTypePtr" and "convertedValuePtr" from "convertedPtr"
                    int converted_length;
                    if (TCL_OK != Tcl_ListObjLength(interp, convertedPtr, &converted_length) ||
                        converted_length != 2) {
                        fprintf(stderr, "converted: %s\n", Tcl_GetString(convertedPtr));
                        Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid typed spec, converted length != 2", -1));
                        return TCL_ERROR;
                    }

                    Tcl_Obj *convertedTypePtr;
                    Tcl_Obj *convertedValuePtr;
                    if (TCL_OK != Tcl_ListObjIndex(interp, convertedPtr, 0, &convertedTypePtr)
                        || TCL_OK != Tcl_ListObjIndex(interp, convertedPtr, 1, &convertedValuePtr)) {
                        Tcl_SetObjResult(interp,
                                         Tcl_NewStringObj("error while extracting type and value from converted",
                                                          -1));
                        return TCL_ERROR;
                    }

                    Tcl_ListObjAppendElement(interp, subListPtr, dictKeyPtr);
                    Tcl_ListObjAppendElement(interp, subListPtr, convertedTypePtr);
                    Tcl_ListObjAppendElement(interp, subListPtr, convertedValuePtr);
                }

                Tcl_Obj *listPtr = Tcl_NewListObj(0, NULL);
                Tcl_ListObjAppendElement(interp, listPtr, Tcl_NewStringObj("document", -1));
                Tcl_ListObjAppendElement(interp, listPtr, subListPtr);
                *resultPtr = listPtr;
            } else {
                Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid type", -1));
                return TCL_ERROR;
            }
            break;
    }
    return TCL_OK;
}

int tjson_TypedToCustom(Tcl_Interp *interp, Tcl_Obj *specPtr, Tcl_Obj **resultPtr) {
    // "specPtr" must be a list with two elements, the first of which must be the type and the second the value
    int length;
    if (TCL_OK != Tcl_ListObjLength(interp, specPtr, &length) || length != 2) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid typed spec, must be a list of two elements", -1));
        return TCL_ERROR;
    }

    Tcl_Obj *typePtr;
    Tcl_Obj *valuePtr;
    if (TCL_OK != Tcl_ListObjIndex(interp, specPtr, 0, &typePtr)
        || TCL_OK != Tcl_ListObjIndex(interp, specPtr, 1, &valuePtr)) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("error while extracting type and value from list", -1));
        return TCL_ERROR;
    }

    int type_length;
    const char *type = Tcl_GetStringFromObj(typePtr, &type_length);

    if (type[0] != 'M') {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("no support in triple notation", -1));
        return TCL_ERROR;
    }

    Tcl_Obj *listPtr = Tcl_NewListObj(0, NULL);

    // "valuePtr" must be a dict
    Tcl_DictSearch search;
    Tcl_Obj *dictKeyPtr;
    Tcl_Obj *dictValuePtr;
    int done;
    if (TCL_OK != Tcl_DictObjFirst(interp, valuePtr, &search, &dictKeyPtr, &dictValuePtr, &done)) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("error while reading dict", -1));
        return TCL_ERROR;
    }
    for (; !done; Tcl_DictObjNext(&search, &dictKeyPtr, &dictValuePtr, &done)) {
        Tcl_Obj *convertedPtr;
        if (TCL_OK != tjson_TypedConvertTypeValueToCustom(interp, dictValuePtr, &convertedPtr)) {
            return TCL_ERROR;
        }

        // extract "convertedTypePtr" and "convertedValuePtr" from "convertedPtr"
        int converted_length;
        if (TCL_OK != Tcl_ListObjLength(interp, convertedPtr, &converted_length) || converted_length != 2) {
            fprintf(stderr, "converted_length: %d\n", converted_length);
            Tcl_SetObjResult(interp, Tcl_NewStringObj("converted must be a list of two elements", -1));
            return TCL_ERROR;
        }

        Tcl_Obj *convertedTypePtr;
        Tcl_Obj *convertedValuePtr;
        if (TCL_OK != Tcl_ListObjIndex(interp, convertedPtr, 0, &convertedTypePtr)
            || TCL_OK != Tcl_ListObjIndex(interp, convertedPtr, 1, &convertedValuePtr)) {
            Tcl_SetObjResult(interp, Tcl_NewStringObj("invalid typed spec", -1));
            return TCL_ERROR;
        }

        Tcl_ListObjAppendElement(interp, listPtr, dictKeyPtr);
        Tcl_ListObjAppendElement(interp, listPtr, convertedTypePtr);
        Tcl_ListObjAppendElement(interp, listPtr, convertedValuePtr);
    }

    *resultPtr = listPtr;
    return TCL_OK;

}