#!/usr/bin/python
# Simple attempt to detect beathing from a microphone.
# Based on a useful example from http://ubuntuforums.org/showthread.php?p=6509055
# to get the audio data from the microphone, and
# http://www.acronymchile.com/sigproc.html to process it.
# with some help from http://stackoverflow.com/questions/9456037/scipy-numpy-fft-frequency-analysis
#
#
# Graham Jones, February 2013.

import alsaaudio, time, audioop
import time
import numpy, scipy, scipy.fftpack
import pylab

inp = alsaaudio.PCM(alsaaudio.PCM_CAPTURE,alsaaudio.PCM_NONBLOCK)

# Set attributes: Mono, 8000 Hz, 16 bit little endian samples
inp.setchannels(1)
inp.setrate(8000)
inp.setformat(alsaaudio.PCM_FORMAT_S16_LE)

# The period size controls the internal number of frames per period.
# The significance of this parameter is documented in the ALSA api.
# For our purposes, it is suficcient to know that reads from the device
# will return this many frames. Each frame being 2 bytes long.
# This means that the reads below will return either 320 bytes of data
# or 0 bytes of data. The latter is possible because we are in nonblocking
# mode.
inp.setperiodsize(160)

analysis_period = 10    # seconds
t_start = time.time()
samples = [];
counter = 0

while True:
	# Collect data for analysis_period seconds, then analyse it.
	if ((time.time()-t_start) > analysis_period):
		print "analysis time!"
		sample_array = numpy.array(samples)
		sample_fft = scipy.fft(samples)
		offt = open('outfile_fft.dat','w')
		freqs = []
		sample_no = []
		sn = 0
		for x in range(len(sample_fft)):
			freq = 1.0*x/analysis_period
			freqs.append(freq)
			sample_no.append(sn)
			sn += 1
			offt.write('%f %f  ' % 
				   (freq,
				    abs(sample_fft[x].real)))

		print len(freqs),len(sample_fft)

		#freqs = scipy.fftpack.fftfreq(signal.size, t[1]-t[0])

		pylab.subplot(211)
		pylab.plot(sample_no, samples)
		pylab.subplot(212)
		pylab.plot(freqs,20*scipy.log10(sample_fft))
		#pylab.xlim(0,freqs[len(sample_fft)/2])
		pylab.xlim(0,100)
		pylab.show()
		t_start = time.time()
	else:
		# Read data from device
		l,data = inp.read()
		counter = 0
		if l:
			print "processing data"
			print data
			while 1:
				try:
					samples.append(audioop.getsample(data,2,counter))
					counter += 1
				except:
					print "counter = %d" % (counter)
					print "end of data"
					break
			# Return the maximum of the absolute value of all samples in a fragment.
			#print samples
			print audioop.max(data, 2)
			#time.sleep(.001)
