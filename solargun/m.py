#!/usr/bin/env python3
# Sol's Stupidly Simple Build System v.2.1
# (c) 2018-2019 Jari Komppa http://iki.fi/sol
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
import subprocess
import sys

assert sys.version_info >= (3, 0)

# Add source files here.
# Special names:
# "*" is a sync point, causing all running processes to finish before
#     continuing, so if you're generating files needed by later sources,
#     add sync point before them.
# "+" is a literal shell command, like "+del foo.bar" runs "del foo.bar".
src = [
    "+cls",
    "crt0.s",
    "sfx.s",
    "ingame.c",
    "app.c",
    "mainmenu.c",
    "gameover.c",    
	]

# Add targets here. Note that if there's dependencies,
# you need the sync ("*") points.
product = [
	"crt0.ihx",
	"*",
	"solargun.tap",
	"*",
	"+speccy solargun.tap"
	]

# Add file suffix based methods here: first param is the resulting
# suffix, second param is the command to execute. Tags are replaced
# before execution: %SRCFILE for source file, %DSTFILE for resulting
# file. %TARGETSUFFIX for lists of target files - so if you generate
# .obj files, %OBJ has list of all object files from src list.
methods = {
	"c" : [ "rel", "..\\cc\\sdcc350\\bin\\sdcc -c -o %DSTFILE %SRCFILE -mz80 --no-std-crt0 --opt-code-speed --Werror --peep-asm --peep-return " ],
	"s" : [ "rel", "..\\cc\\sdcc350\\bin\\sdasz80 -xlos -g %DSTFILE %SRCFILE" ],
	"ihx" : [ "ihx", "..\\cc\\sdcc350\\bin\\sdcc -mz80 --no-std-crt0 --opt-code-speed --nostdlib --code-loc 0x8032 --data-loc 0xe000 -Wl -b_HEADER=0x8000 %REL"],
	"tap" : [ "tap", "..\\tools\\mackarel crt0.ihx %DSTFILE SolarGun -nosprestore -noei -lowblock logo.scr 0x5b00"]
	}

# If you want to add tags, just add keys here,
# like "SDCC": "../sdcc350/bin/sdcc":
objectlists = {}

#####################################################################
#####################################################################
#####################################################################

class BuildTask:
	def __init__(self, proc, dstfile):
		self.proc = proc
		self.dstfile = dstfile

procs = []

changed = False

# If we want to track includes from .s files, we just need to
# make a variant of this..

def scandeps_c(srcfile, reftime, ignore = None):
	# Avoid infinite loops
	if ignore is None:
		ignore = set()
	if srcfile not in ignore:
		ignore.add(srcfile)
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
				dtime = scandeps_c(x[x.find('"')+1:x.rfind('"')], reftime, ignore)
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
	print("building " + dstfile)
	if os.path.isfile(dstfile):
		os.remove(dstfile)
	cmd = method.replace("%SRCFILE", srcfile).replace("%DSTFILE", dstfile)
	for key, value in objectlists.items():
		cmd = cmd.replace("%"+key.upper(), value)
	procs.append(BuildTask(subprocess.Popen(cmd, shell=True, bufsize=8192, stdout=subprocess.PIPE, stderr=subprocess.STDOUT), dstfile))
	changed = True

def sync():
	global procs
	failboat = False
	livecount = 0
	for x in procs:
		if x.proc.poll() is None:
			livecount += 1
	# If more than one process is still running, print info.
	if livecount > 1:
		print("Waiting for", len(procs), "processes to finish..")
	for x in procs:
		(a,b) = x.proc.communicate()
		t = a.decode('ascii')
		# avoid printing empty lines..
		if t != "":
			print(t)
		if not os.path.isfile(x.dstfile):
			print("Error: " + x.dstfile + " not generated.")
			failboat = True
	procs = []
	if failboat:
		exit()

#####################################################################
#####################################################################
#####################################################################

# scan all source files, build if changed
for x in src:
	if x[0] == "*":
		sync()
	else:
		if x[0] == "+":
			os.system(x[1:])
		else:
			if not os.path.isfile(x):
				print(x + " does not exist.")
				exit()
			fname = x[:x.rfind(".")]
			suffix = x[x.rfind(".")+1:]
			if suffix not in methods:
				print("Don't know how to handle files of type " + suffix + " (in " + x + ").")
				exit()
			if methods[suffix][0] not in objectlists:
				objectlists[methods[suffix][0]] = fname + "." + methods[suffix][0];
			else:
				objectlists[methods[suffix][0]] += " " + fname + "." + methods[suffix][0];
			if agecheck(x, fname + "." + methods[suffix][0]):
				build(x, fname + "." + methods[suffix][0], methods[suffix][1])

# Sync before moving to product phase
sync()

# If targets don't exist, build them
for x in product:
	if x[0] not in "*+" and not os.path.isfile(x):
		changed = True

# if source files have changed, or targets don't change, link
if changed:
	for x in product:
		if x[0] == "*":
			sync()
		else:
			if x[0] == "+":
				os.system(x[1:])
			else:
				fname = x[:x.rfind(".")]
				suffix = x[x.rfind(".")+1:]
				if suffix not in methods:
					print("Don't know how to handle files of type " + suffix + " (in " + x + ").")
				build(x, fname + "." + methods[suffix][0], methods[suffix][1])
else:
	print("Nothing to do.")
	exit()

# Final sync just to be sure
sync()

print("All done.")
