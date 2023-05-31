#!/usr/bin/env python
#coding=utf-8
import cv2

cap = cv2.VideoCapture(0)

while True:
    ret, frame = cap.read()
    cv2.imshow("Test", frame)
    key = cv2.waitKey(1)
    if key == ord('q'):
        break
