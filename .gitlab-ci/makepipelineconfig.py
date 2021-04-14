#!/usr/bin/env python3

import os
import sys
import json
import string
from argparse import ArgumentParser

# require Python 3
if sys.version_info.major < 3:
    sys.exit('Python 3 required')

parser = ArgumentParser(description='Generate dumux test pipeline .yml file')
parser.add_argument('-o', '--outfile', required=True,
                    help='Specify the file to write the pipeline definition')
parser.add_argument('-c', '--testconfig', required=False,
                    help='Specify a test configuration file containing the '
                         'tests that should be run within the test pipeline')
parser.add_argument('-t', '--template', required=False,
                    default='.gitlab-ci/default.yml.template',
                    help='Specify the template .yml file to be used')
parser.add_argument('-i', '--indentation', required=False, default=4,
                    help='Specify the indentation for the script commands')
args = vars(parser.parse_args())


# substitute content from template and write to target
def substituteAndWrite(mapping):

    template = args['template']
    if not os.path.exists(template):
        sys.exit("Template file '" + template + "' could not be found")

    with open(args['outfile'], 'w') as ymlFile:
        raw = string.Template(open(template).read())
        ymlFile.write(raw.substitute(**mapping))


commandIndentation = ' '*args['indentation']
duneConfigCommand = 'dunecontrol --opts=$DUNE_OPTS_FILE --current all'
with open(args['outfile'], 'w') as ymlFile:

    def makeScriptString(commands):
        commands = [commandIndentation + '- ' + comm for comm in commands]
        return '\n'.join(commands)

    def makeMultiLineCommand(commandParts):

        # add indentation to each part
        result = [commandIndentation + '  ' + cp for cp in commandParts]

        # add line break token at the end of each part
        result = ' \\\n'.join(result)

        # add multiline trigger token at the beginning
        return '|\n' + result

    # if no configuration is given, build and run all tests
    if not args['testconfig']:
        buildCommand = [duneConfigCommand,
                        'dunecontrol --opts=$DUNE_OPTS_FILE --current '
                        'make -k -j4 build_tests']
        testCommand = ['cd build-cmake', 'dune-ctest -j4 --output-on-failure']

    # otherwise, parse data from the given configuration file
    else:
        with open(args['testconfig']) as configFile:
            config = json.load(configFile)

            testNames = list(config.keys())
            targetNames = [tc['target'] for tc in config.values()]

            if not targetNames:
                buildCommand = [duneConfigCommand,
                                'echo "No tests to be built."']
            else:
                # The MakeFile generated by cmake contains .NOTPARALLEL,
                # as it only allows a single call to `CMakeFiles/Makefile2`.
                # Parallelism is taken care of within that latter Makefile.
                # We let the script create a small custom makeFile here on top
                # of `Makefile2`, defining a new target to be built in parallel
                buildCommand = [
                    duneConfigCommand,
                    'cd build-cmake',
                    'rm -f TestMakefile && touch TestMakefile',
                    'echo "include CMakeFiles/Makefile2" >> TestMakefile',
                    'echo "" >> TestMakefile',
                    makeMultiLineCommand(['echo "build_selected_tests:"']
                                         + targetNames
                                         + ['>> TestMakefile']),
                    'make -f TestMakefile -j4 build_selected_tests']

            if not testNames:
                testCommand = ['echo "No tests to be run, make empty report."',
                               'cd build-cmake',
                               'dune-ctest -R NOOP']
            else:
                testCommand = ['cd build-cmake',
                               makeMultiLineCommand(['dune-ctest -j4 '
                                                     '--output-on-failure '
                                                     '-R']
                                                    + testNames)]

    substituteAndWrite({'build_script': makeScriptString(buildCommand),
                        'test_script': makeScriptString(testCommand)})
