# KLP KAIRA Local Pipeline

Software to allow the reception and recording of LOFAR/CEP packets.

Original LOFAR system by ASTRON
Packet formatting design by Arno Schoenmaker, et al., ASTRON
The KLP software:
Based on the original code "udptest" by James Anderson, MPIfR
v0.1 26-Nov-2012, Juha Vierinen, SGO, & Derek McKay, RAL
v1.0 15-May-2015, Ilkka Virtanen, U.Oulu
v1.1 02-Oct-2015, Ilkka Virtanen, U.Oulu
v1.4 20-Mar-2024, Derek McKay, Aalto

## INTRODUCTION

The purpose of this programme is to allow simple local access to the
LOFAR data pipeline (the four 1 Gbit lanes coming out of the LOFAR
system). The programme provides a simple framework to parse the incoming
UDP data packets [1] and provide customized processing for the incoming
data stream. This initial release will provide the several processing
back ends:

- Store beamlet data to disk 
- Store averaged power
- Do nothing (this is useful for testing the data transfer
  capabilities of a computer) 

The core of the programme has two threads. One of the threads receives 
UDP packets coming the from the LOFAR RSP, parses them and stores them 
in a double buffer. Another user defined thread then is called whenever 
the buffer is filled up. This simple framework allows users to easily 
implement their own processing pipelines using ANSI C.

The processing is currently handled on a per-lane basis, so one instance 
of the programme can access only up to 61 beamlets. This is by design. 
Running multiple pipelines, possibly on multiple computers will allow 
the processing of multiple lanes simultaneously. If communication 
between pipelines processing different lanes is needed, this has to be 
implemented by the user, e.g., using standard IPC libraries, such as 
MPI.

There are other programmes with similar capabilities, but the KLP 
framework is designed to be minimalist to allow users to start modifying 
their own data recorder software, without needing to invest time into 
the larger frameworks. If sophisticated highly-configurable 
object-oriented software is needed for processing LOFAR data, you should 
probably be considering core LOFAR software or something akin to PELICAN 
[2].


## REQUIREMENTS

Since it was originally written, the (augmented) C language has morphed 
slightly. While the core C language itself remains essentially 
unchanged, the POSIX, UNIX, BSD, Linux, OPEN and various other standards 
have been updated. Then the glibc implementation of these standards has 
evolved. The original KLP was written for Linux Ubuntu 10.04. KLP 1.0 
was ported to MacOSX. This version returns to Ubuntu (22.04).

KLP v1.4 has been tested and works on:

   gcc (Ubuntu 11.4.0-1ubuntu1~22.04) 11.4.0



## INSTALL 

KLP requires a Linux system. Installation procedure is as follows:

   tar xvfz klp-1.4.tar.gz
   cd klp-1.4
   make
   sudo make install



## WRITING A PIPELINE

The nullsink.c or filewriter.c pipelines are fairly simple
demonstrations on how to write your own pipeline. 

A processing pipeline has to implement two functions:

#include "klp_core.h"

/* Initialization is called once at startup 

   (void *)d->userdata   Can be used to store arbitrary 
                         data for the pipeline (see filewriter.c).
           d->buffersize Can be set at init and it defines the
                         size of the buffer that is passed to klp_proc
                         the default is 390625 samples.
 */
void klp_init(int argc, char **argv, recorder_t *d);

/* 
   process data block is called every time that a datablock is
   completed 

   d->buffer->datax[i] Contains the x polarization data vector for the
                       ith  beamlet. 
   d->buffer->datay[i] Contains the x polarization data vector for the
                       ith  beamlet

   The data vectors are 16 bit little endian integers with real and imaginary
   parts interleaved, just as they come out of the RSP.

   d->buffersize       Tells you how many samples are in the
                       buffer. By default this is 390625 samples 
                       per beamlet.
*/
void klp_proc(recorder_t *d);


## OLD HARDWARE NOTES

The first version of this software that was used at Kilpisjärvi 
Atmospheric Imaging Receiver Array (KAIRA) made use of a computer 
(kaira01, that had access to the LOFAR RSP output lanes) with a standard 
six core Intel i7 PC with 24 GB of memory and an Intel Quad port 
PCIExpress ethernet card interfacing the four 1 Gbit LOFAR data lanes.

While testing the pipeline at KAIRA, sometimes dropped packets (1 
dropped packet out of 5 million packets on average) occurred. This 
occurred even when no heavy processing was attached to the pipeline. 
After some troubleshooting, it was discovered that disabling the 
InterruptThrottleRate feature of the kernel Intel ethernet driver 
resolved the problem. To do this, first unload the existing module:

> rmmod e1000e

and then install the module again with the following options:

> modprobe e1000e InterruptThrottleRate=0,0,0,0

With this setting, it is possible to operate four simultaneous data 
lanes at full speed using a single PC (ca. 2010 vintage), as long as the 
computer is capable of processing the data. The UDP packet handling and 
parsing causes approximately 50% of the CPU time of one processor core. 
So far we have not lost a single packet after using these kernel module 
settings.

Since then, KLP was moved to a MacPro (kaira05, ca.2015) that had 
sufficient capability for this not to be an issue. At that time changes 
were made to KLP to port it to the MacOSX operating system.


Also on older hardware (ca. 2010) it may be necessary to increase buffer 
size limits. This was required for Ubuntu 10.04, but not for MacOSX 
(2014). To do this, use the following.

   sudo sysctl -w net.core.rmem_default=100000000
   sudo sysctl -w net.core.rmem_max=100000000




## REFERENCES

[1] Virtanen, "Station Data Cookbook", LOFAR-ASTRON-MAN-064
[2] https://github.com/pelican and http://www.oerc.ox.ac.uk/research/artemis
[3] McKay-Bukowski, &a, "KAIRA: the Kilpisjärvi Atmospheric Imaging Receiver
    Array - system overview and first results", doi:10.1109/TGRS.2014.2342252

## ACKNOWLEDGEMENT

The authors acknowledge contributions from various people from the 
KAIRA and LOFAR communities. In particular: James Anderson, Menno Norden, 
Arno Schoenmakers, Teun Grit, Klaas Stuurwold, Olaf Wucknitz and 
Ilkka Virtanen.

The construction of the KAIRA facility was funded by the Infrastructure 
Funds of the University of Oulu, together with European Regional 
Development Funds of Lapland through the Regional Council of Lapland 
including contributions from Municipality of Sodankylä, as well as by 
the 7th Framework Preparatory Phase project "EISCAT_3D: A European 
Three-Dimensional Imaging Radar for Atmospheric and Geospace Research".


## LICENSE

FreeBSD License

Copyright (c) 2012, Juha Vierinen, Derek McKay
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met: 

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer. 
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution. 

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The views and conclusions contained in the software and documentation are those
of the authors and should not be interpreted as representing official policies, 
either expressed or implied, of the authors of this software.

