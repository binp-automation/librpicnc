#!/usr/bin/python3

raw = []

with open("Still Alive Abs.txt") as file:
	for line in file:
		data = line.split()
		if len(data) >= 5 and (data[1] == "On" or data[1] == "Off"):
			raw.append(data)

seq = []

class Note:
	def __init__(self, chan, pos, dur, num, vol):
		self.chan = chan
		self.pos = pos
		self.dur = dur
		self.num = num
		self.vol = vol
	
	def __str__(self):
		return "{ chan: %d, pos: %d, dur: %d, num: %d, vol: %d }" % (self.chan, self.pos, self.dur, self.num, self.vol)

def parse(b, e):
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
	note = parse(raw[2*i], raw[2*i + 1])
	if note != None:
		seq.append(note)
		# print(note)

chan=1
print("chan1[][3] = {")
for note in seq:
	if note.chan != chan:
		chan = note.chan
		print("};\nchan%d[][3] = {" % chan)
	print("{%d, %d, %d}," % (note.pos, note.dur, note.num))
print("};")