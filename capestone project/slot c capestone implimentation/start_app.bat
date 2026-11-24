@echo off
title Smart Organ Donation Matching System
echo ======================================================================
echo   Smart Organ Donation Matching System - Capstone Project
echo   Starting Python HTTP Server & C Matching Engine on Port 5000...
echo ======================================================================
cd /d "%~dp0"

python server\server.py 5000
pause
