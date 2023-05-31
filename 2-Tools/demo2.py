#!/usr/bin/env python
#coding=utf-8
import cv2

cap = cv2.VideoCapture(0)

cap.set(cv2.CAP_PROP_CONVERT_RGB, 0)

while True:
    ret, frame = cap.read()
    frame = frame << 6
    cv2.imshow("Test", frame)
    key = cv2.waitKey(1)
    if key == ord('q'):
        break
