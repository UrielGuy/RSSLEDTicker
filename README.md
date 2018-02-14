# RSSLEDTicker
Use multiple cheap LED matrixes with an arduino and a raspberry pi to create a news ticker at home.
This uses [this|https://www.ebay.com/itm/Provide-Arduino-code-64x16-dot-Matrix-LED-for-diy-Sign-Light-Neon-Bright-UNO-MCU-/271303628009] kind of pretty cheap LED matrixes to create a large (tested with 3) news ticker from any RSS feed.
Originally I tried to adjust the code from [here|http://www.instructables.com/id/64x16-RED-LED-Marquee/] but couldn't get it to work with more than a single panel, and also didn;t like the way letters just pop into existenxe at the end rather than slide in. 

Code is highly specilized for rolling text - it does not support any other display on the LED matrix, although it can be added relatively easily. 

The Arduino constantly draws the matrix (on main loop, not interrupts because it just worked without). There is a frame bufer that contains the displayed frame, plus another 8 pixels for each row. Whenever we scroll a letter we will pop the next letter into the 8 columns that aren't currently displayed (hence the larger buffer) so they scrol in smoothly.

The make sure the scrolling is continous, the Arduino controlling the matrix has a buffer of the next 128 characters. Every second it will send out on the serial port how many free bytes it has in the buffer. The raspberry pi then reads that and send the appropriate amount of chars to the Arduino so to not overflow the buffer. 

The Raspberry Pi shell script (Should work on any linux box, serial port file name might be different) uses the rsstail utility (you might need to run 'sudo apt-get install rsstail'), formats all of the rss feeds (CNN and Fox news by default) into a single string and sends it slowly for display. When done, it does that again. 

