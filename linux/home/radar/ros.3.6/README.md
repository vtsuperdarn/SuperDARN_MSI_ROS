Guide for Using/Compiling this code.

Maybe for now this is a scratch-pad area to jot down useful commands in a pinch.  Some of this is
gleened from the Operators Guide to Software Controls.


Compiling the Code

Whenever making changes to the source code for a binary file, in order to apply the
changes of the soruce code to the binary file, the code needs to be compiled through one
of the means below.

Making the Whole RST Code on the Linux Computer

From time to time it may be necessary to remake all of the code that is on the linux computer. In
order to do this, login to the linux computer and type:

make.code superdarn ros.3.6

If you're having trouble with this command, then its possible a new/different version of
the code is being used.  The two input parameters for make.code are the project (here
superdarn) and the package (here ros.3.6).  To figure out more here, echo the
environment variabel RPKG with "echo ${RPKG}". The result will give you the
directory listing such that:

~/xxx/rpkg/project/package/

If you don't get anything here, then you may need to check on your environment
variables in the ~/.profile file.
