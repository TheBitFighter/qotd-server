A small server that implements RFC 865 Quote of the day protocol.

If a TCP request is received via port 17 (if not otherwise specified) a random quote from the 
given text file is selected and send as a response. 
A specific client can only request a not yet chosen number of quotes to mitigate spamming.

The code originates from a university assignment and is not yet fully functional in the way intended.
Some conversion and documentation effort still has to be conducted.
