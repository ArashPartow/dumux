#!/usr/bin/env python3
# SPDX-FileCopyrightInfo: Copyright © DuMux Project contributors, see AUTHORS.md in root folder
# SPDX-License-Identifier: GPL-3.0-or-later


import os
import json
import argparse
import sys

if sys.version_info[0] < 3:
    sys.exit("Python 3 or a more recent version is required.")

from convert_code_to_doc import *

class Navigation:
    """Add navigation bars to examples with subpages"""

    def __init__(self, config, dir):
        # read and check the configuration
        if "mainpage" not in config:
            raise IOError("The .doc_config navigation section has to contain a mainpage entry!")
        mainpage = config["mainpage"]
        if "subpages" not in config:
            raise IOError("The .doc_config navigation section has to contain a subpages entry!")
        if len(config["subpages"]) < 1:
            raise IOError("The .doc_config navigation:subpages array has to have at least one entry!")
        subpages = config["subpages"]
        if "subtitles" not in config:
            subtitles = ["Part {} of the documentation".format(i+1) for i in range(len(subpages))]
        else:
            if len(config["subtitles"]) != len(subpages):
                raise IOError("In the .doc_config navigation section the subpages array has to have the same length as the subtitles array!")
            subtitles = ["Part {}: {}".format(i+1, t) for i, t in enumerate(config["subtitles"])]
        self.dir = dir # the working directory

        # create the snippets to insert into the docs
        self.snippets = {}

        # first the navigation snippet for the main page
        self.snippets[mainpage] = { "header" : "", "footer" : "" }
        makeHead = lambda t: "\n## {}\n\n".format(t)
        makeEntry = lambda i,l: "| [:arrow_right: Click to continue with part {} of the documentation]({}) |\n|---:|\n\n".format(i,l)
        self.snippets[mainpage]["footer"] = "".join(
            [ makeHead(title) + makeEntry(index+1, self.getRelPath(mainpage, subpage)) for index, (subpage, title) in enumerate(zip(subpages, subtitles))]
        ).rstrip() + '\n' # end file with a single endline character

        # then the navigation snippets for the subpages
        pagetypes = ["mid" for i in range(len(subpages))]
        pagetypes[0] = "first"
        pagetypes[-1] = "last"
        for index, (subpage, pagetype) in enumerate(zip(subpages, pagetypes)):
            self.snippets[subpage] = { "header" : "", "footer" : "" }
            if pagetype == "mid":
                self.snippets[subpage]["header"] = " ".join(["\n|",
                    "[:arrow_left: Back to the main documentation](" + self.getRelPath(subpage, mainpage) +  ") |",
                    "[:arrow_left: Go back to part {}]".format(index) + "(" + self.getRelPath(subpage, subpages[index-1]) +  ") |",
                    "[:arrow_right: Continue with part {}]".format(index+2) + "(" + self.getRelPath(subpage, subpages[index+1]) +  ") |"
                ]) + "\n|---|---|---:|\n\n"
            elif pagetype == "last":
                self.snippets[subpage]["header"] = " ".join(["\n|",
                    "[:arrow_left: Back to the main documentation](" + self.getRelPath(subpage, mainpage) +  ") |",
                    "[:arrow_left: Go back to part {}]".format(index) + "(" + self.getRelPath(subpage, subpages[index-1]) +  ") |"
                ]) + "\n|---|---:|\n\n"
            else: # first subpage
                self.snippets[subpage]["header"] = " ".join(["\n|",
                    "[:arrow_left: Back to the main documentation](" + self.getRelPath(subpage, mainpage) +  ") |",
                    "[:arrow_right: Continue with part {}]".format(index+2) + "(" + self.getRelPath(subpage, subpages[index+1]) +  ") |"
                ]) + "\n|---|---:|\n\n"
            # for subpages header and footer are the same
            self.snippets[subpage]["footer"] = self.snippets[subpage]["header"]

    def getRelPath(self, page, target):
        absPageBasePath = os.path.split(os.path.abspath(os.path.join(self.dir, page)))[0]
        absTargetPath = os.path.abspath(os.path.join(self.dir, target))
        return os.path.relpath(absTargetPath, absPageBasePath)

    def header(self, page):
        return self.snippets[page]["header"]

    def footer(self, page):
        return self.snippets[page]["footer"]


def convertToMarkdownAndMerge(dir, config, navigation=None):

    for target, sources in config.items():

        targetExtension = os.path.splitext(target)[1]
        if not targetExtension == ".md":
            raise IOError("Markdown files expected as targets! Given target: {}".format(target))

        targetPath = os.path.join(dir, target)
        os.makedirs(os.path.dirname(targetPath), exist_ok=True)

        with open(targetPath, "w") as targetFile:
            thisScript = os.path.basename(__file__)
            targetFile.write("<!-- Important: This file has been automatically generated by {}. Do not edit this file directly! -->\n\n".format(thisScript))
            if navigation:
                targetFile.write(navigation.header(target))
            for source in sources:
                fileExtension = os.path.splitext(source)[1]
                if fileExtension == ".md":
                    with open(os.path.join(dir, source), "r") as markdown:
                        targetFile.write(markdown.read())
                elif fileExtension == ".hh" or fileExtension == ".cc":
                    with open(os.path.join(dir, source), "r") as cppCode:
                        sourceRelPath = os.path.relpath(os.path.abspath(os.path.join(dir, source)), os.path.split(os.path.abspath(targetPath))[0])
                        targetFile.write("\n\n" + transformCode(cppCode.read(), cppRules(), sourceRelPath) + "\n")
                else:
                    raise IOError("Unsupported or unknown file extension *{}".format(fileExtension))
            if navigation:
                targetFile.write(navigation.footer(target))

def generateReadme(dir):
    config = None
    # look for .doc_config, if not found we pass
    try:
        configname = os.path.join(dir, ".doc_config")
        with open(configname, 'r') as configFile:
            config = json.load(configFile)
    except FileNotFoundError:
        pass
    if config is not None:
        navigation = None
        if "navigation" in config:
            navigation = Navigation(config.pop("navigation"), dir)
        convertToMarkdownAndMerge(dir, config, navigation)

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument("-d", "--directory", help="The folder to look for examples", default=".")
    args = vars(parser.parse_args())

    for path in os.listdir(args["directory"]):
        abspath = os.path.join(os.path.abspath(args["directory"]), path)
        if os.path.isdir(abspath):
            generateReadme(abspath)
