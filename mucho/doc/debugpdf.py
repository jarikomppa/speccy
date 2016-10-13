""" builds documentation files from multimarkdown (mmd) source
    to various formats, including the web site and pdf.
"""

import subprocess
import glob
import os
import sys
import time
import shutil

src = [
    "intro.mmd", 
    ]

website_only = [
    "downloads.mmd"
    ]

unknown = 0
for file in glob.glob("*.mmd"):
    if file not in src:
        unknown = 1
        print file + " not included in docs!"
        
if unknown:
    print "Add the new files to makedoc.py, main.tex and htmlpre.txt."
    sys.exit()

datestring = time.strftime("%Y%m%d")
if not os.path.exists(datestring + "/web"):
  os.makedirs(datestring + "/web")
if not os.path.exists("temp/"):
  os.makedirs("temp/")
   
print "- -- --- -- - Generating LaTex"

for x in src:
    if x not in website_only:
        subprocess.call(["pandoc", "-t", "latex", "--listings", "--default-image-extension=pdf", "--chapters", x, "-o", "temp/" + x[:len(x)-3]+"tex.orig"])
        with open("temp/" + x[:len(x)-3]+"tex", "w") as file_out:
            with open("temp/" + x[:len(x)-3]+"tex.orig", "r") as file_in:
                for line in file_in:
                    file_out.write(line.replace('\\begin{longtable}[c]{@{}ll@{}}', '\\begin{tabulary}{\\textwidth}{lJ}').replace('\\begin{longtable}[c]{@{}lll@{}}', '\\begin{tabulary}{\\textwidth}{lJJ}').replace('\\begin{longtable}[c]{@{}llll@{}}', '\\begin{tabulary}{\\textwidth}{lJJJ}').replace('\\endhead','').replace('\\end{longtable}','\\end{tabulary}'))
 
print "- -- --- -- - Generating pdf (xelatex_output.txt)"

subprocess.call(["xelatex", "main.tex"])
subprocess.call(["xelatex", "main.tex"])

shutil.move("main.pdf", datestring + "/doc_" + datestring + ".pdf")

print "- -- --- -- - Done - " + datestring
