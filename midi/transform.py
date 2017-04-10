#!/usr/bin/python3

raw = []

with open("still_alive_abs.txt") as file:
	for line in file:
		data = line.split()
		if len(data) >= 5 and (data[1] == "On" or data[1] == "Off"):
			raw.append(data)

notes = []

class Note:
	def __init__(self, chan, pos, dur, num, vol):
		self.chan = chan
		self.pos = pos
		self.dur = dur
		self.num = num
		self.vol = vol
	
	def __str__(self):
		return "{ chan: %d, pos: %d, dur: %d, num: %d, vol: %d }" % (self.chan, self.pos, self.dur, self.num, self.vol)

def parseNote(b, e):
	if b[1] != "On" or e[1] != "Off":
		print("%s != On or %s != Off" % (b[1], e[1]))
		return None
	if b[2] != e[2] or b[3] != e[3]:
		print("%s != %s or %s != %s" % (b[2], e[2], b[3], e[3]))
		return None
	if e[4] != "v=0":
		print("%s != v=0" % e[4])
		return None
	chan = int(b[2].split("=")[1])
	pos = int(b[0])
	dur = int(e[0]) - pos
	num = int(b[3].split("=")[1])
	vol = int(b[4].split("=")[1])
	return Note(chan, pos, dur, num, vol)

for i in range(len(raw)//2):
	note = parseNote(raw[2*i], raw[2*i + 1])
	if note != None:
		notes.append(note)
		# print(note)

chan=1
prefix = "static const int chan%d[] =";
postfix = "0, 0, 0"
print(prefix % chan)
print("{")
for note in notes:
	if note.chan != chan:
		chan = note.chan
		print(postfix)
		print("};")
		print(prefix % chan)
		print("{")
	print("%d, %d, %d," % (note.pos, note.dur, note.num))
print(postfix)
print("};")

"""
lyrics = []

class Lyric:
	def __init__(self, pos, text):
		self.pos = pos;
		self.text = text;

	def __str__(self):
		return "{ pos: %d, text: %d }" % (self.pos, self.text)

with open("still_alive_abs.txt") as file:
	for line in file:
		words = line.split()
		if len(words) >= 4 and words[1] == "Meta" and words[2] == "Lyric":
			text = line.split("\"")[1];
			lyrics.append(Lyric(int(words[0]), text));

prefix = "static const char *lyrics[] =";
postfix = "NULL"
print(prefix)
print("{")
for lyric in lyrics:
	print("(const char *) %d, \"%s\"," % (lyric.pos, lyric.text))
print(postfix)
print("};")
"""
