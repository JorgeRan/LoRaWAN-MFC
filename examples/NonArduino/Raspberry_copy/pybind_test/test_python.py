import subprocess
import time


p = subprocess.Popen(["./controller"], stdin=subprocess.PIPE, stdout=subprocess.PIPE, text=True)

print(p.stdout.readline().strip())

while True:
	cmd = input("Enter command: ")
	if cmd == "q":
		break
	
	cmd_2 = 'MFC 2'
	
	p.stdin.write(cmd + "-" + cmd_2 + "\n")
	p.stdin.flush()
	
	print(p.stdout.readline().strip())
