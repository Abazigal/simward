Simward
=======

Command line program for easy TCP / UDP port forwarding with (optional) automatic data replacement.
This tool is written in C, and uses a single thread to do its job.

So far, forwarding (TCP/UDP) is working.
Data replacement will be available in a later release ...

## Usage
* Launch the tool
<pre>
    Usage: simward [-t|-u] [-m &lt;max&gt;] 
          &lt;local_port&gt; &lt;remote_host&gt; &lt;remote_port&gt;
    
    Options:
        -t Use TCP mode (default)
        -u Use UDP mode
        -m Maximum number of simultaneous forwards (default: 10)
</pre>

    For example:
<pre>
    simward -t -m 15 1234 www.google.fr 80
</pre>


* Keep an eye on active forwards
<pre>
<b>&gt; list</b>
Active TCP forwards (2/15 max):
&nbsp;
= Forward ID 5 =
Members: 127.0.0.1:40266 &lt;-&gt; 80.93.81.178:80
State: Waiting for data
Forwarded: 74.94 Mbytes
Last activity: &lt; 1 sec
&nbsp;
= Forward ID 7 =
Members: 192.168.1.12:40270 &lt;-&gt; 80.93.81.178:80
State: Waiting for data
Forwarded: 866.98 Kbytes
Last activity: &lt; 1 sec
&nbsp;
<b>&gt; kill 5</b>
Forward 5 stopped.
&nbsp;
<b>&gt; quit</b>
Closing ...
</pre>

