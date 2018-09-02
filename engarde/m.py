# Sol's Stupidly Simple Build System v.1.1
# (c) 2018 Jari Komppa http://iki.fi/sol
#
# This software is provided 'as-is', without any express or implied
# warranty. In no event will the authors be held liable for any damages
# arising from the use of this software.
#
# Permission is granted to anyone to use this software for any purpose,
# including commercial applications, and to alter it and redistribute it
# freely, subject to the following restrictions:
# 
# 1. The origin of this software must not be misrepresented; you must not
#    claim that you wrote the original software. If you use this software
#    in a product, an acknowledgment in the product documentation would be
#    appreciated but is not required.
# 2. Altered source versions must be plainly marked as such, and must not be
#    misrepresented as being the original software.
# 3. This notice may not be removed or altered from any source distribution.

import os

# Add source files here:
src = [
    "crt0.s",
    "primdraw.c",
    "data.c",
    "drawstring.c",
    "ingame.c",
    "tour.c",
    "heal.c",
    "newcard.c",
    "shop.c",
    "app.c"
	]

# Add targets here:
product = ["crt0.ihx", "engarde.tap"]

# Add file suffix based methods here: first param is the resulting
# suffix, second param is the command to execute. Tags are replaced
# before execution: %SRCFILE for source file, %DSTFILE for resulting
# file. %TARGETSUFFIX for lists of target files - so if you generate
# .obj files, %OBJ has list of all object files from src list.
methods = {
	"c" : [ "rel", "..\\cc\\sdcc360\\bin\\sdcc -c -o %DSTFILE %SRCFILE -mz80 --no-std-crt0 --opt-code-speed --Werror --peep-asm --peep-return " ],
	"s" : [ "rel", "..\\cc\\sdcc360\\bin\\sdasz80 -xlos -g %DSTFILE %SRCFILE" ],
	"ihx" : [ "ihx", "..\\cc\\sdcc360\\bin\\sdcc -mz80 --no-std-crt0 --opt-code-speed --nostdlib --code-loc 0x6007 --data-loc 0x5b00 -Wl -b_HEADER=0x6000 %REL"],
	"tap" : [ "tap", "..\\tools\\mackarel crt0.ihx %DSTFILE Engarde loader.scr -nosprestore -noei"]
	}

# If you want to add tags, just add keys here, 
# like "SDCC": "../sdcc350/bin/sdcc":
objectlists = {}

#####################################################################
#####################################################################
#####################################################################

changed = False

def clear():
	os.system("cls")

# If we want to track includes from .s files, we just need to
# make a variant of this..
	
def scandeps_c(srcfile, reftime, recursion = 1):
	# Avoid infinite loops
	if recursion < 8:
		# It's ok if we don't find the file, we'll just skip it
		if os.path.isfile(srcfile):
			dtime = os.path.getmtime(srcfile)
			if (dtime > reftime):
				reftime = dtime
			# Scan for includes and recurse
			inc = []
			with open(srcfile, "r") as f:
				for line in f:
					if len(line.strip()) > 0 and line.strip()[0] == "#" and "include" in line and '"' in line:
						inc.append(line)
			for x in inc:
				dtime = scandeps_c(x[x.find('"')+1:x.rfind('"')], reftime, recursion + 1)
				if (dtime > reftime):
					reftime = dtime
	# Return the biggest timestamp found
	return reftime
	
def scandeps(srcfile, reftime):
	if srcfile[srcfile.rfind(".")+1:] in "c h cpp hpp".split():
		return scandeps_c(srcfile, reftime)
	return reftime
	
def agecheck(srcfile, dstfile):
	if not os.path.isfile(dstfile):
		return True
	srctime = scandeps(srcfile, os.path.getmtime(srcfile))
	dsttime = os.path.getmtime(dstfile)
	if (srctime > dsttime):
		return True
	return False
	
def build(srcfile, dstfile, method):
	global changed
	print "building " + dstfile
	if os.path.isfile(dstfile):
		os.remove(dstfile)
	cmd = method.replace("%SRCFILE", srcfile).replace("%DSTFILE", dstfile)
	for key, value in objectlists.iteritems():
		cmd = cmd.replace("%"+key.upper(), value)
	os.system(cmd)
	if not os.path.isfile(dstfile):
		print dstfile + " not generated."
		exit()
	changed = True
	
clear()

# scan all source files, build if changed
for x in src:
	if not os.path.isfile(x):
		print x + " does not exist."
		exit()
	fname = x[:x.rfind(".")]
	suffix = x[x.rfind(".")+1:]
	if suffix not in methods:
		print "Don't know how to handle files of type " + suffix + " (in " + x + ")."
		exit()
	if methods[suffix][0] not in objectlists:
		objectlists[methods[suffix][0]] = fname + "." + methods[suffix][0];
	else:
		objectlists[methods[suffix][0]] += " " + fname + "." + methods[suffix][0];
	if agecheck(x, fname + "." + methods[suffix][0]):
		build(x, fname + "." + methods[suffix][0], methods[suffix][1])

for x in product:
	if not os.path.isfile(x):
		changed = True
		
# if source files have changed, link
if changed:
	for x in product:
		fname = x[:x.rfind(".")]
		suffix = x[x.rfind(".")+1:]
		if suffix not in methods:
			print "Don't know how to handle files of type " + suffix + " (in " + x + ")."
		build(x, fname + "." + methods[suffix][0], methods[suffix][1])
else:
	print "Nothing to do."
	exit()

print "All done."
