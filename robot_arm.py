import time
import cv2
from matplotlib import pyplot as plt
import numpy as np
from Arm7Bot_py.lib.Arm7Bot import Arm7Bot

arm = Arm7Bot('COM5')
file_path = "C:/Users/Lenovo/Desktop/test.txt"

while(1):
    openfile = open(file_path)
    content = openfile.readlines()
   # print(content)
    if len(content)==0:
        continue
    x = content[0].split(" ")[1]
    y = content[0].split(" ")[2]
    z = content[0].split(" ")[3]

    openfile.close()
    file=open(file_path,'w+')
    file.close()

    print(x)
    print(y)
    print(z)

    # arm.setIK6([int((pr[0][0]+pr[1][0])/2), int((pr[0][1]+pr[1][1])/2), 150], [0, 0, -1])  # move to [x=0,y=200,=150]
    # arm.setIK6([110,250,150],[0,0,-1])
    # time.sleep(1)
    # arm.setIK6([int((pr[0][0]+pr[1][0])/2), int((pr[0][1]+pr[1][1])/2), 30], [0, 0, -1])  # down

    arm.setAngle(6, 90)  # open hand
    time.sleep(1)
    arm.setAngle(6, 18)  # close hand
    time.sleep(1)
