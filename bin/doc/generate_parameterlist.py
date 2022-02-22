#!/usr/bin/env python3

"""
Automatically updates parameterlist.txt by searching all *.hh files
for usage of getParam or getParamFromGroup.
"""

import os
import argparse
import json
import logging
import sys

# setup a logger
logger = logging.getLogger(__name__)
LOG_LEVEL = logging.INFO
try:
    LOG_LEVEL = getattr(logging, os.environ["DUMUX_LOG_LEVEL"].upper())
except KeyError:
    pass
except AttributeError:
    logger.warning("Invalid log level in environment variable DUMUX_LOG_LEVEL")

logger.setLevel(LOG_LEVEL)
loggingFileHandler = logging.FileHandler("generate_parameterlist.log", mode="w")
loggingFormatter = logging.Formatter("%(levelname)-8s %(message)s")
loggingFileHandler.setFormatter(loggingFormatter)
loggingFileHandler.setLevel(LOG_LEVEL)
logger.addHandler(loggingFileHandler)


class ErrorLoggerCounted:
    """A logger wrapper to count the number of errors"""

    def __init__(self, method):
        self.method = method
        self.counter = 0

    def __call__(self, *args, **kwargs):
        self.counter += 1
        return self.method(*args, **kwargs)


logger.error = ErrorLoggerCounted(logger.error)
logger.info(
    "Please fix all ERRORs, WARNINGs may require attention, INFO/DEBUG is just information."
)
logger.info("--------------------------------------------------------------------------------")


def getEnclosedContent(string, openKey, closeKey):
    """find the content of the given string between
    the first matching pair of opening/closing keys"""

    # cut off everything before the first occurrence of openKey
    string = openKey + string.partition(openKey)[2]

    # get content between mathing pair
    rest = string.partition(closeKey)
    result, rest = rest[0] + closeKey, rest[2]
    while result.count(openKey) != result.count(closeKey):
        rest = rest.partition(closeKey)
        if rest[1] == "":
            raise IOError(
                f"Could not get content between '{openKey}'"
                f" and '{closeKey}' in given string '{string}'"
            )
        result, rest = result + rest[0] + closeKey, rest[2]

    return result.partition(openKey)[2].rpartition(closeKey)[0]


def extractParameterName(line):
    """extract a parameter from a given line"""

    # split occurrences of getParam<T>(CALLARGS) or getParamFromGroup<T>(CALLARGS)
    # into the template arguments T and the function arguments CALLARGS
    if "getParamFromGroup<" in line:
        line = line.split("getParamFromGroup")[1]
        hasGroupPrefix = True
    elif "getParam<" in line:
        line = line.split("getParam")[1]
        hasGroupPrefix = False
    else:
        return {}

    # this is a current limitation (fix this if it starts to occur)
    if line.count("getParam") > 1:
        raise IOError('Cannot process multiple occurrences of "getParam" in one line')

    # remove trailing spaces and cut off everything behind semicolon
    line = line.strip("\n").strip(" ").split(";")[0]

    # extract template arg between '<' and '>'
    paramType = getEnclosedContent(line, "<", ">")

    # extract function arguments
    functionArgs = line.partition("<" + paramType + ">")[2]
    functionArgs = getEnclosedContent(functionArgs, "(", ")")

    if hasGroupPrefix:
        functionArgs = functionArgs.partition(",")[2]
    functionArgs = functionArgs.partition(",")
    parameterName = functionArgs[0]
    defaultValueArg = None if not functionArgs[2] else functionArgs[2]

    paramType = paramType.strip(" ")
    parameterName = parameterName.strip(" ")
    if defaultValueArg:
        defaultValueArg = defaultValueArg.strip(" ")

    # if we get an empty name
    if len(parameterName) == 0:
        raise IOError("Could not correctly process parameter name")
    # if interior spaces occur in the parameter name, we can't identify it
    if parameterName[0] != '"' or parameterName[-1] != '"' or " " in parameterName:
        raise IOError("Could not correctly process parameter name")

    return {
        "paramType": paramType,
        "paramName": parameterName.strip('"'),
        "defaultValue": defaultValueArg,
    }


def getParameterListFromFile(fileName):
    """extract all parameters from a given file"""
    parameterList = []
    errors = {}
    with open(fileName) as paramsFile:
        for lineIdx, line in enumerate(paramsFile):
            try:
                param = extractParameterName(line)
                if param:
                    parameterList.append(param)
            except IOError as exc:
                errors[lineIdx] = {"line": line.strip(), "message": exc}

    # print encountered errors
    if errors:
        logger.warning(
            f"{len(errors)} parameter(s) in file {fileName}"
            " could not be retrieved automatically."
            " Please check them..."
        )
        for lineIdx, errorInfo in errors.items():
            logger.warning(f"\t-> line {lineIdx}: {errorInfo['line']}")
            logger.warning(f"\t\t-> error message: {errorInfo['message']}")

    return parameterList


class CheckExistAction(argparse.Action):
    """check if the input file exists"""

    def __call__(self, parser, namespace, values, option_strings=None):
        if os.path.isfile(values):
            setattr(namespace, self.dest, values)
            setattr(namespace, "hasInput", True)
        else:
            parser.error(f"File {values} does not exist!")


argumentParser = argparse.ArgumentParser(
    description="""
    This script generates parameters list from header files.
    The header files "test" and "examples" folders are not included.
    ----------------------------------------------------------------
    If input file is given, the descriptions will be copied from the input.
    Multientry of parameters are allowed in input files.
    """,
    formatter_class=argparse.RawDescriptionHelpFormatter,
)
argumentParser.add_argument(
    "--root",
    help="root path of Dumux",
    metavar="rootpath",
    default=os.path.abspath(os.path.join(os.path.abspath(__file__), "../../../")),
)
argumentParser.add_argument(
    "--input",
    help="json file of given paratemers",
    action=CheckExistAction,
    metavar="input",
    dest="inputFile",
    default=os.path.abspath(
        os.path.join(
            os.path.dirname(os.path.abspath(__file__)), "../../doc/doxygen/extradoc/parameters.json"
        )
    ),
)
argumentParser.add_argument(
    "--output",
    help="relative path (to the root path) of the output file",
    metavar="output",
    default="doc/doxygen/extradoc/parameterlist.txt",
)
cmdArgs = vars(argumentParser.parse_args())

# search all *.hh files for parameters
logger.info("Searching for parameters in the source file tree")
logger.info("--------------------------------------------------------------------------------")
parameters = []
rootDir = cmdArgs["root"]
for root, dirs, files in os.walk(rootDir):
    dirs[:] = [d for d in dirs if d not in ("test", "examples")]
    for file in files:
        if os.path.splitext(file)[1] == ".hh" and os.path.splitext(file)[0] != "parameters":
            parameters.extend(getParameterListFromFile(os.path.join(root, file)))
logger.info("--------------------------------------------------------------------------------")

# make sorted dictionary of the entries
# treat duplicates (could have differing default values or type names - e.g. via aliases)
parameterDict = {}
for params in parameters:
    key = params["paramName"]
    if key in parameterDict:
        parameterDict[key]["defaultValue"].append(params["defaultValue"])
        parameterDict[key]["paramType"].append(params["paramType"])
    else:
        parameterDict[key] = params
        parameterDict[key]["defaultValue"] = [params["defaultValue"]]
        parameterDict[key]["paramType"] = [params["paramType"]]

# get the explanations from current parameter json file
inputDict = {}
if cmdArgs["inputFile"]:
    with open(cmdArgs["inputFile"], "r") as f:
        data = f.read()
        inputDict = json.loads(data)

# add the missing parameters from input
missingParameters = [key for key in inputDict if key.replace("-.", "") not in parameterDict]

for missingKey in missingParameters:

    parameter = inputDict[missingKey]["group"] + "." + inputDict[missingKey]["parameter"]

    hasMode = "mode" in inputDict[missingKey].keys()
    MODE = ""
    if hasMode:
        MODE = inputDict[missingKey]["mode"]
    if MODE == "manual":
        key = missingKey.replace("-.", "")
        parameterDict[key] = inputDict[missingKey]
        parameterDict[key]["defaultValue"] = inputDict[missingKey]["default"]
        parameterDict[key]["paramType"] = inputDict[missingKey]["type"]
        parameterDict[key]["paramName"] = parameter

        logger.info(
            f"Added parameter '{parameter}' to the parameter list. The parameter"
            " could not be extracted from code"
            f" but has been explicitly added in {cmdArgs['inputFile']}"
        )

    else:
        logger.error(
            f"Found parameter '{parameter}' in {cmdArgs['inputFile']}"
            f" which has not been found in the code "
            "--> Set mode to 'manual' in the input file if it is to be kept otherwise delete it!"
        )

parameterDict = dict(sorted(parameterDict.items(), key=lambda kv: kv[0]))
# determine actual entries (from duplicates)
# and determine maximum occurring column widths

tableEntryData = []
for key in parameterDict:

    entry = parameterDict[key]
    hasGroup = entry["paramName"].count(".") > 0
    group = "-" if not hasGroup else entry["paramName"].split(".")[0]
    parameter = entry["paramName"] if not hasGroup else entry["paramName"].partition(".")[2]

    # In case of multiple occurrences,
    # we prefer the default value from input
    # otherwise use the first entry that is not None
    # and write the others in log for possible manual editing
    # determin multiple entries in input
    keyInput = group + "." + parameter
    NUM_ENTRIES = 0
    if keyInput in inputDict:
        NUM_ENTRIES = max(
            len(inputDict[keyInput]["default"]),
            len(inputDict[keyInput]["type"]),
            len(inputDict[keyInput]["explanation"]),
        )

    parameterTypeName = entry["paramType"][0]
    defaultValue = next((e for e in entry["defaultValue"] if e), "-")

    hasMultiplePT = not all(pt == parameterTypeName for pt in entry["paramType"])
    hasMultipleDV = not all(
        dv == (defaultValue if defaultValue != "-" else None) for dv in entry["defaultValue"]
    )
    if hasMultiplePT or hasMultipleDV:
        writeLog = logger.error if NUM_ENTRIES == 0 else logger.debug
        writeLog(
            f"Found multiple occurrences of parameter {parameter}"
            " with differing specifications: "
        )
        if hasMultiplePT:
            writeLog(" -> Specified type names:")
            for typeName in entry["paramType"]:
                writeLog(f"        {typeName}")
            if NUM_ENTRIES == 0:
                writeLog(" ---> Please provide parameter type manually in input file!")
            else:
                parameterTypeName = inputDict[keyInput]["type"]
                writeLog(
                    f" ---> For the parameters list, {parameterTypeName}"
                    " has been chosen. Type is from input file."
                )
        if hasMultipleDV:
            writeLog(" -> Specified default values:")
            for default in entry["defaultValue"]:
                if default:
                    writeLog(f"        {default}")
                else:
                    writeLog("        - (none given)")
            if NUM_ENTRIES == 0:
                writeLog(" ---> Please provide default value manually in input file!")
            else:
                defaultValue = inputDict[keyInput]["default"]
                writeLog(
                    f" ---> For the parameters list, {defaultValue}"
                    " has been chosen. Default value is from input file."
                )

    if NUM_ENTRIES == 0:
        tableEntryData.append(
            {
                "group": group,
                "name": parameter,
                "type": parameterTypeName,
                "default": defaultValue,
                "explanation": "",
            }
        )
    else:
        explanationMsg = inputDict[keyInput]["explanation"]
        parameterTypeName = inputDict[keyInput]["type"] if hasMultiplePT else [parameterTypeName]
        defaultValue = inputDict[keyInput]["default"] if (hasMultipleDV or defaultValue == "-") else [defaultValue]
        for i in range(NUM_ENTRIES):
            # maybe fewer entries for some keys
            if len(defaultValue) < i + 1:
                defaultValue.append(defaultValue[i - 1])
            if len(explanationMsg) < i + 1:
                explanationMsg.append(explanationMsg[i - 1])
            if len(parameterTypeName) < i + 1:
                parameterTypeName.append(parameterTypeName[i - 1])

            tableEntryData.append(
                {
                    "group": group,
                    "name": parameter,
                    "type": parameterTypeName[i],
                    "default": defaultValue[i],
                    "explanation": explanationMsg[i],
                }
            )

# generate actual table entries
tableEntriesWithGroup = []
tableEntriesWithoutGroup = []
PREVIOUS_GROUP_ENTRY = None

GROUP_ENTRY_LENGTH = 20
PARAM_NAME_LENGTH = 45
PARAM_TYPE_LENGTH = 24
DEFAULT_VALUE_LENGTH = 15
EXPLANATION_LENGTH = 150


def tableEntry(groupEntry, paramName, paramTypeName, defaultParamValue, explanation):
    """Create a table entry for a parameter"""
    return (
        f" * "
        f"| {groupEntry.ljust(GROUP_ENTRY_LENGTH)} "
        f"| {paramName.ljust(PARAM_NAME_LENGTH)} "
        f"| {paramTypeName.ljust(PARAM_TYPE_LENGTH)} "
        f"| {defaultParamValue.ljust(DEFAULT_VALUE_LENGTH)} "
        f"| {explanation.ljust(EXPLANATION_LENGTH)} |"
    )


for data in tableEntryData:

    groupName = data["group"]
    if groupName != PREVIOUS_GROUP_ENTRY:
        PREVIOUS_GROUP_ENTRY = groupName
        if groupName != "-":
            groupName = "\\b " + groupName

    if len(data["explanation"].strip()) == 0:
        logger.error(
            f"Parameter {groupName}.{data['name']} has no explanation. Add it to the input file!"
        )

    TABLE_ENTRY = tableEntry(
        groupEntry=groupName,
        paramName=data["name"],
        paramTypeName=data["type"],
        defaultParamValue=data["default"],
        explanation=data["explanation"],
    )
    if groupName != "-":
        tableEntriesWithGroup.append(TABLE_ENTRY)
    else:
        tableEntriesWithoutGroup.append(TABLE_ENTRY)

# combine entries
tableEntries = tableEntriesWithGroup + tableEntriesWithoutGroup

HEADER = """/*!
 *\\internal
 * ****** W A R N I N G **************************************
 * This file is auto-generated. Do not manually edit.
 * Run bin/doc/generate_parameterlist.py
 * ***********************************************************
 *\\endinternal
 *
 *\\file
 *\\ingroup Parameter
 *
 *\\brief List of currently useable run-time parameters
 *
 * The listed run-time parameters are available in general,
 * but we point out that a certain model might not be able
 * to use every parameter!
 *\n"""
HEADER += " * | " + "Group".ljust(GROUP_ENTRY_LENGTH)
HEADER += " | " + "Parameter".ljust(PARAM_NAME_LENGTH)
HEADER += " | " + "Type".ljust(PARAM_TYPE_LENGTH)
HEADER += " | " + "Default Value".ljust(DEFAULT_VALUE_LENGTH)
HEADER += " | Explanation ".ljust(EXPLANATION_LENGTH) + "|\n"

HEADER += " * | " + ":-".ljust(GROUP_ENTRY_LENGTH)
HEADER += " | " + ":-".ljust(PARAM_NAME_LENGTH)
HEADER += " | " + ":-".ljust(PARAM_TYPE_LENGTH)
HEADER += " | " + ":-".ljust(DEFAULT_VALUE_LENGTH)
HEADER += " | :- ".ljust(EXPLANATION_LENGTH) + "|\n"

HEADER += " * | " + "-".ljust(GROUP_ENTRY_LENGTH)
HEADER += " | " + "ParameterFile".ljust(PARAM_NAME_LENGTH)
HEADER += " | " + "std::string".ljust(PARAM_TYPE_LENGTH)
HEADER += " | " + "executable.input".ljust(DEFAULT_VALUE_LENGTH)
HEADER += " | :- ".ljust(EXPLANATION_LENGTH) + "|\n"

# overwrite the old parameterlist.txt file
PARAMETER_FILE_NAME = os.path.join(rootDir, cmdArgs["output"])
logger.info("--------------------------------------------------------------------------------")
logger.info(f"Overwriting parameter file in {PARAMETER_FILE_NAME}")
logger.info("--------------------------------------------------------------------------------")
with open(PARAMETER_FILE_NAME, "w") as outputfile:
    outputfile.write(HEADER)
    for e in tableEntries:
        outputfile.write(e + "\n")
    outputfile.write(" */\n")

if logger.error.counter > 0:
    print("Finished with errors! Check log file!")
    logger.error(f"Counted {logger.error.counter} error message lines. Please fix the errors!")
    sys.exit(1)
else:
    print(f"Successfully create new parameter list at {PARAMETER_FILE_NAME}")
    logger.info("Finished without errors")
