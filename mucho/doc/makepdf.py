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
    "intro.mmd" 
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
    subprocess.call(["pandoc", "--listings", "--default-image-extension=pdf", "--chapters", x, "-o", "temp/" + x[:len(x)-3]+"tex"])

print "- -- --- -- - Generating pdf (xelatex_output.txt)"

#with open('xelatex_output.txt', 'w') as outfile:
#    subprocess.call(["xelatex", "main.tex"], stdout=outfile)
#    print "- -- --- -- - Generating pdf pass 2.."
#    subprocess.call(["xelatex", "main.tex"], stdout=outfile)
subprocess.call(["xelatex", "main.tex"])
print "- -- --- -- - Generating pdf pass 2.."
subprocess.call(["xelatex", "main.tex"])

shutil.move("main.pdf", datestring + "/doc_" + datestring + ".pdf")

print "- -- --- -- - Cleanup.."
tempsuffix = ["aux", "toc", "out", "log", "lg", "4ct", "4tc", "idv", "tmp", "xdv", "xref", "bak"]
for suffix in tempsuffix:
    for file in glob.glob("*."+suffix):
        os.remove(file)
    for file in glob.glob(datestring + "/web/*."+suffix):
        os.remove(file)
for file in glob.glob("temp/*"):
   os.remove(file)
os.rmdir("temp")

print "- -- --- -- - Done - " + datestring
