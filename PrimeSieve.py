#!/usr/bin/python3
# vi: ts=4 sw=4 et ai sm
# (c) Copyright 2011 Robert Uzgalis

# Version of 2011-5-11

#             BZET Primes Example
# Primes are difficult for compression because they stay
# relatively dense. BZETS do remarkably well in the long
# run with only a mild cost in compression when compared to
# a run-length encoded bit-set.  And depending on the size
# of the set about the same as recording the bits without
# compression, or in the case of primes upto 100,001 a yield
# of a 10% reduction in size.  For primes upto 1,000,000
# the reduction would probably be a %30 reduction in size over
# the raw bits.

# from rawC import Raw as BZET
# from Bzet2 import BZET
# from Bzet4 import BZET
# from Bzet8 import BZET
from RLE     import rle as BZET
from time import *
from math import sqrt

def cpssn( bs ):
    return (bs/bmsize)*100.0

bl      = 6                  # Run with bottom level = 64 bits.
#bl      = 1                  # Run with bottom level = 2 bits.
#BZET.NewBLevel(bl)           # Set the bottom level.

maxsize = 10000
report = 1000
npp = 100           # Number of primes to print
notprime = BZET(0)  # Zero is not a prime 
Pmask = BZET([(2, maxsize-1)])
#print( "Pmask is", Pmask)
print( "notprime is", notprime)
print( BZET.Version() )
print( "Starting at", ctime(time()))
print( "setting max size to", maxsize )
print( "setting report freq", report )
print( "print first", npp, "primes" )
bmsize = maxsize/8.0
bsize = notprime.size()
print( "Starting bitset size uncompressed", bmsize, "bytes; compressed", \
       bsize, "%4.2f"%cpssn(bsize), "%\n" ) 


# Do Sieve of Eratosthenes to find Primes.

nextp = 1
count = 0
stm = clock()
last_tm = stm
limit = int(sqrt(maxsize)) + 1
print( "limit is:", limit )
while nextp < limit:    
    if count%report == 1 or count < 25:
        bsize = notprime.size()
        tc = clock()        
        print( "%6i"%count, "%7.1f"%(tc-last_tm), "sec; prime:", \
               "%3i;"%nextp, "size", "%6d"%bsize, "%6.2f"%cpssn(bsize) + \
               "%" )
        last_tm = tc
    count += 1    
    for np in range(nextp+1,maxsize):
        # Find the next prime
        if not notprime[np]:
            nextp = np
            break
    # notprime |= BZET( [ ( nextp*nextp, maxsize, nextp-1 ), ] )    
    for ix in range( nextp*nextp, maxsize, nextp ):
        notprime[ix] = True
    #print( "finished prime:", nextp, "count", notprime.COUNT(), "size", notprime.size())

print( "\nAt prime", count, nextp, "there are no more primes less than", \
       maxsize, "\n" )
#print( "Pmask is", Pmask )
#print( "Notprime is", notprime )
primes = notprime.NOT()    # Get the primes
#print( "Primes is", primes )
primes = Pmask.AND(primes) # Dont let 1 or maxsize be prime
                           # This also cuts out any surious
                           # bits from the NOT
#print( type(primes) )
count = primes.COUNT()     # Count the one bits
bsize = notprime.size()    # Final size
ftm = clock()              # Final time.
ftm -= stm                 # If this is not running on Windows
print( "Ending at", ctime(time()), "%7.1f"%ftm, "seconds" )
h = ftm // 3600            # hours of computing
xtm = ftm - (h * 3600 )
m = xtm // 60              # minutes of computing
s = xtm % 60               # seconds of computing 
lastp = primes.LAST()
print( "that is: ", "%2d"%h, "h", "%2d"%m, "m", "%2d"%(int(s+0.5)), "s" )
print( "%5d"%count, "%7.1f"%ftm, "sec; last prime:", \
               "%6d"%lastp, "; final size:", bsize, ("%6.2f"%cpssn(bsize)) + \
               "%; primes/sec", "%5.2f"%(float(count)/ftm) )

# Print Table of Primes

ix = 0
for p in primes.LIST_T(limit=npp):
    if p > maxsize: break
    if ix%10 == 0: print( "" )
    ix += 1
    print( "%6d"%p, "", end='' )
    
    

