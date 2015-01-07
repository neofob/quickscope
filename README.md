
Installing: See file INSTALL.  This is a standard GNU autotools package.
In a shell run: './configure'; 'make'; and 'make install'.

To build it from the github repository source files run the 'bootstrap'
script before configure.

So build/install it and then run **qs_demo_launcher**.  qs_demo_launcher
will let you get a sample of what quickscope can do.


### Run: qs_demo_launcher


### User C Code Examples


The examples in examples/ do not use GNU autotools to build.  They are
single file code examples meant to be used with the installed quickscope
package (this package).  They come with a GNU make file that builds any
examples/*.c file into a quickscope enabled program.  You can write a
quickscope oscilloscope program by calling just 4 C API functions.


### If You Like What You See


If you like what you see, keep in mind that quickscope it currently
unstable.  That does not necessarly means it's buggy, but just that it may
be subject to have interface changes.  If you like you can contact me,
Lance, the developer through email, or whatever means that GitHub.com
provides.  I don't like spam.  Email me at: lance.arsenault   _AT_ that
big evil search company that does not give a shit about net neutrality,
but used to, dot COM.  Hint: 10^100mail.com  or you may just find my
email address by 10^100ing it.



----------------------------------------------------------------------

----------------------------------------------------------------------


The rest of this document is just development notes, me being stupid,
thinking out load.  Quickscope is in pre-Alpha. Many features are still
missing, but it currently does a lot.  We don't know of any bugs currently
in it.  Half the time I think I see a bug it's just a dumb user problem,
like a low input sample rate not being compatible with a high sweep rate.
There needs to be more code in it to check for stuff like that.  Like on a
real oscilloscope if you turn the wrong knob you see no display and you
futz around for 5 minutes trying to recover a good display only to find
out you paused the input, but by that time all the parameters are total
crap.



#### Developer Notes


This file: contains general information about Quickscope and development
notes with arguments about it's design.  Quickscope is a research project
that is spinning off a currently usable software package.  Currently just
runs on GNU/Linux systems.


Quickscope is computer programs and one or more libraries written in C.
It has bindings for other languages (planed), a shell interface (planed),
and GTK+ widgets.  It is an attempt at making a generic 2-D real-time
operator-in-the-loop display tool.  Something like a real oscilloscope but
with the flexibility of software not encumbered by limitations of
hardware, and then again not as fast a real scope.  We don't care if the
buttons and knobs don't necessarily look like a real scope, and to that
end we extend parameter controls beyond that of buttons and knobs given
the flexibility of software.


When you consider how slow a computer pixel monitor and device input rates
you quickly conclude that it will be a very slow oscilloscope when
compared to even a very old 2MHz oscilloscope from the 1950's.  We decided
that if we could read and display most data from a 44kHz sample source,
like a sound card, without using a lot of computer resources, then we'd
consider pursuing this software oscilloscope.  In 2012 we wrote some code
and attempted to display all the sound data coming in at ~44kHz, and two
of the four computer's cores where pegged and it could barely do it, if
you did not challenge the computer with other tasks.  At this point you
are saying to yourself there are lots of software scopes that can do
this.  We know this, but we are not interesting in making a simple
sub-sampling display, sweeping refresh software scope.  We are making
generic fading beam scope that has X-Y mode as well as triggered and free
running sweep modes.  Each frame displayed has any number of virtual beam
traces, with any number of inputs, be there triggers or not.  We wish to
make a frame work for building any kind of scope with any kind of computer
input.  We decided that if we could display a fading beam like a real
scope, then it would be interesting enough to pursue.  We will consider
the non-fading beam, sub-sampling wiping sweep that other software
scopes currently use as some kind of degenerate mode of our fading beam
scope.  Two years later (2014) we got a new hand-me-down computer (Intel
Quad Core i5-2500 3.30GHz CPU, 32GB DDR3) that was barely fast enough to
pass our bench mark and so Quickscope was born.  We suspect that the X
server rendering code improved too, which we have nothing to do with.
Also older scope code may have just been stupid things.

We just use libX11's XDrawPoints() to draw.  Using such a simple primitive
means that we add a bit more code, but we found this necessary in order to
make this magical fading beam and have it fast enough to be useful.

OpenGL: We didn't bother trying to make this scope code faster by writing
openGL code and using direct rendering.  We didn't see much point to that
given that the current bottle neck is in the fading beam code and by
letting the X Server do the rendering it can be rendered in parallel
without having to write more code, though it may be a little delayed.  The
X server seems to be using inter process shared memory to transfer the
pixel points that need to be drawn.  We have two drawing methods, buffered
and unbuffered, which is just using an X pixmap or not respectively.  Both
buffered and unbuffered seem to have some merit, depending on inputs and
rates used.  It appears that the X server is using openGL via Xrender, so
why do we need to reinvent the wheel, so to speak.  We are literally just
drawing pixel points because we found that was faster than drawing
lines.

More on OpenGL (swap buffers):  For standard 3-D graphics apps there is
great benefit in drawing to two buffers and swapping them, given that
every single pixel may be unrelated to the corresponding one in the
previous frame buffer.  In the case of quickscope pixel rendering, most
of the pixels in temporally adjacent frames will be the same, it's
guaranteed within some limit of the pixel input rate and pixel fade rate.
That is the nature of buffer frames between adjacent frames is very
different for 3-D graphics and quickscope.  In extreme cases of low sample
rates quickscope could only be changing 100 pixels per (60 Hz) refresh
frame and so "swap buffering" would just increase system resources by
orders of magnitude.  So please don't tell me "this is stupid because
you're not using advanced graphics hardware", until you think about it
first.  In this low rates limit it's the different between pegging the CPU
like 3-D graphics programs tend to do and 0.2 % CPU when just drawing a
few pixels per frame.  In the upper limit when the scope display fills
most of the pixels with it's beam trace, it would not be a useful scope.
All the trace lines in the scope would be mashed together letting you see
just blobs and not lines.  So the nature of a oscilloscope is to draw thin
the distinct lines, which means that it's not drawing on most of the view
port area.

Drawing pixels with OpenGL drawing will still be tested and compared with
libX11's XDrawPoints() (todo).

Comparing with video play back: To start with playing video has a big
advantage of knowing what to draw an infinite amount of time ahead.  With
quickscope we can only benefit from pre computing view port pixels at the
expense of display delay.  A 1/4 second delay in the display (from the
time of input) would look very bad in quickscope, but for video play back
a 1/4 of a second delay is very short and likely not even possible, and
would be great. This delay is unrelated to the time between video
frames.  So this interactive time needs to be kept in mind when comparing
quickscope with video play back.  Because of this added latency in lining
up buffer frames, it's not obvious how quickscope can benefit from video
play back methods.  I was going to look at the video code, but I now see
that would only help for a scope that showed the traces about one second
after the data was read, and was not what we are interested in.  Latency
between time of data input to the time of display much be small.  That
not the case for video place play back.

We do not use Cario to draw, as GTK+ does.  We found it to be way to slow.
We don't bother with anti-aliasing when drawing the beam, because we found
that that was way to slow.  We do a little anti-aliasing when drawing the
grid lines, which is not as a big resource usage compared to anti-aliasing
the beam lines.  The beam is only one pixel wide to minimize the system
resource usage.  Higher fidelity drawing things can be made optional, but
we will keep things as fast as possible by default.

The resource usage, CPU and memory usage, depends heavily on the size of
the view port (window drawing areas).  Using 3 small view ports uses less
system resources than a single full screen view port.  

The one interesting performance/fidelity consideration is the use of
drawing and hardware monitor refresh synchronizing.  By using the X server
to do the drawing we are also letting the X server determine when it
draws, and we are not synchronizing data sampling with the drawing.
Using gtk_widget_add_tick_callback() may do this.  Consider making a new
kind of controller to test this. Okay... Done.  Seems to work a little
better (~5% less cpu usage) than the interval timer method (interval
class).  Was not game changer (5%).

We use long double to measure time so that programs can run a long time
(say 10 years or so).   A program running 10 years, keeping time in
seconds will use log2(3600 * 24 * 365 * 10) = 28+ bits of the floating
point number leaving 53-29 = 22 bits (2^22 = 4194304) for a double (if
we used a double) which is just about enough, but we wish to push this
limit given that higher sample rates will be expected in the future.
Being able to offset the time at regular intervals would get around this
at the expense of adding or fixing a glitch in the display at regular
intervals.  So ya, time is a long double, even though we may not need it
most of the time.  On Linux amd64 systems gettimeofday() returns two 64
bit ints, one for seconds and one for fractions of seconds.  Currently not
all that resolution is used.  It's clearly there, so that, it works well in
the future.  We are building Quickscope for the future too.



## Class/Object structure:


Managing an object means that the manager destroys the object when the
manager is destroyed.

We use QS_ASSERT() and QS_VASSERT() almost everywhere we can.  We zero
memory every time before freeing it, which in combination with QS_ASSERT()
and QS_VASSERT() catches most memory management bugs.  They go away in
non-debug builds.  In the long run it saves development time, especially
when refactoring, which is unavoidable.  Even gods have limited
foresight, because code has free will.

C is not C++. We don't have/use the complexity of GObject, we chose a less
robust and less rigorous method of a the idea of C objects.  It's less
code and better performance, at the expense of making refactoring a very
hard problem.  The table of definitions below should be kept up to date
with the code, so as to keep object concepts straight in the developers
mind, since we are using a very lose definition of object in quickscopes'
code.  Cross file (not static) structs and functions must follow a naming
convention for when to use '_qs', 'qs' or 'Qs' prefix and when to
camel-case or '_' separate names.  The top rule being: names of private,
and/or protected, internal functions, which are not part of the external
API (application programming interface) have a '_qs' prefix.  API public
(published) function names start with 'qs'.  Sometimes we expose interal
interfaces in installed API header files, in order to have faster inline
code, especially when interating in tight inner loops.  Since performance
is paramount it just can't be avoided.



Class         Description
----------------------------------------------------------

<dl>

<dt>
app
</dt>     <dd>global singlet object, parses command line
              options, keeps a default parameters settings
              that are used as objects are created and can
              have default settings changed just before
              objects that use them are created, has a
              timer object, app manages win and controller
              objects, destroying app will destroy every
              object in Quickscope</dd>
<dt>
controller 
</dt>     <dd>base class for drawsync, interval and fd,
              manages sources, makes sense to have just one
              per program but can have more, calls sources'
              read callbacks, controller sets up a blocking
              call that is called regularly which in turn
              calls the sources' read callbacks regularly</dd>

<dt>
drawsync
</dt>     <dd>a controller that uses
              gtk_widget_add_tick_callback() so it should
              be calling the source read callbacks at each
              draw frame for an associated win object, the
              associated win manages this object, as does
              app</dd>

<dt>
interval
</dt>     <dd>a controller that manages a single callback
              interval using g_timeout_add_full(), which
              I imagine is using the system family of
              functions that are started with setitimer()</dd>

<dt>
fd
</dt>     <dd>a controller that reads files, uses something
              like a glib wrapper of a blocking poll() call
              call using a g_source, used for efficient
              blocking read of a file,<dd>

<dt>
idle
</dt>     <dd>This controller uses g_idle_add() to keep
              looping and reading sources.  One of the
              source read callbacks should do a blocking
              read or like thing to keep this loop from
              running uncontrolled and using too much CPU
              usage</dd>
<dt>
source
</dt>     <dd>we read data from a source, source callback
              controlled by controller using a controller,
              this allocates the buffer of data read that
              may be used to render in the beam trace,
              sources cause traces to draw when they get new
              data, a source is an adjusterList, win adds
              adjusters from the sources in its traces to
              its adjusterList, app manages source objects,
              controllers that control sources manage sources,
              destroying a source will automatically destroy
              traces that use that source

              Time is always increasing or the same between
              frames, but there can be frames which have
              multiple channel values.  That makes is possible
              to have dependent channels that add move values
              that would otherwise not be there, like adding
              a "pen lift" when we sweep back in a "sweep source".</dd>
<dt>
source types
</dt>      <dd>
              <p><i>periodic</i>: fixed periodic, variable periodic,
                  or selectable periodic.  All have a maximum
                  and minimum period. The time interval between
                  frames, period, is fixed in a given source
                  read callback cycle.</p>

              <p><i>fixed periodic</i>: the period is fixed for the
                  life of the source.  This is the most rigid
                  type.</p>

               <p><i>variable periodic</i>: the period can vary continuously
                  between source read callback cycles.</p>

               <p><i>selectable periodic</i>: the period can be varied
                  to a selected list of values between source
                  read callback frames.</p>

              <p><i>tolerant</i>: the time interval between frames
                  is not necessarily predictable.  This source
                  can tolerate any interval and is compatible
                  with periodic sources. It has a min and max
                  sample rate that a variable periodic source
                  will override.  It has a sample rate that is
                  overridden if there are other source types.</p>

              <p><i>custom</i>: the time interval between frames
                  is not necessarily predictable. This source
                  is does not tolerate any other source type.
                  Source type compatibility between custom sources
                  is handled with each source implementation.
                  custom is a catch all that lets you do what ever
                  you want at the expense of having to write more
                  code and not share the same controller that
                  non-custom sources are using.</p>

              <p>When scope under-run occurs, pen lifts will
              automatically be added, and time shifts added
              to periodic and tolerant sources before the read
              callback is called; not so for custom sources.</p>
              </dd>

<dt>
group
</dt>     <dd>a group of sources.  Has no public interfaces
              and is used internally only.  Is managed by the
              sources that belong to the group.  Source types
              in a group must be compatible.  The source group
              has an adjusterList with widgets to adjust things
              for all the sources in the group, like frame rate</dd>

<dt>
sweep
</dt>     <dd>is a source that acts like a scope sweep with
              trigger, it has a source read callback that reads
              another source buffer to compose the sweep source,
              has lots of configuration parameters that can
              be adjusted and are in its source adjusterList</dd>

<dt>
trace
</dt>     <dd>managed by a win, short for beam trace,
              has an X source and Y source,
              qsSource_addTraceDraw() connects to a source
              to get it's draw callback called in source's
              read callbacks after the source reads data</dd>

<dt>
win
</dt>     <dd>GTK+ window widget with drawing area,
              manages associated traces, manages associated
              drawsyncs, is an adjusterList with widgets you
              can interact with, we draw pixels in the drawing
              area GTK widget without using the GTK+ API since
              GTK+ does not support fast pixel drawing,
              currently using libX11 to draw, OpenGL will
              also be tried too</dd>

<dt>
adjuster
</dt>     <dd>generic thingy for adjusting a parameter</dd>

<dt>
adjusterList
</dt>     <dd>a list of adjusters, manages many adjuster
              objects, two adjusterLists may be added together
              making bigger adjusterLists</dd>

<dt>
widget
</dt>     <dd>a widget that displays/inputs adjusters to
              and from the user, not to be confused with a
              GTK widget, it is managed by one adjusterList,
              it back and forth through the adjusterList
              displaying each adjuster one at a time,
              it, there may be any number of widgets displaying
              an adjusterList</dd>

<dt>
timer
</dt>     <dd>simple gettimeofday() or clock_gettime() wrapper,
              there's one in app, turns time in seconds into a
              long double</dd>

<dt>
assert
</dt>     <dd>QS_ASSERT() and QS_VASSERT() macros
              that are automatically removed if
              configure --enable-debug is not set</dd>


</dl>



#### On-the-fly Widget Changeable Parameter Types

|type         |       example                          |
|-------------|----------------------------------------|
|long double  | sweep Delay, sweep Holdoff (times)     |
|double       | trace plot scale and shift             |
|float        | fadePeriod, fadePeriod                 |
|bool         | points lines freeRun                   |
|selector     | like radio selector, ex: sweep slope   |



##### Shell notes:

A command shell interface is warranted.  A shell that connects to the
running scope with a pipe or socket.  This would be a quicker and more
usable interface than stupid graphical widgets.  Not as obvious but much
more powerful.  With tab completion and history from GNU readline.

The things controlled in a connection are listed in this table.


|Class     | On-the-fly Changeable Parameters  |
|----------|:----------------------------------|
|win       | fadeDrawPeriod fadePeriod drawPeriod backgroundColor gridColor showGrid showAxis show... |
|trace     | points lines pointColor lineColor xScale xShift yScale yShift |
|interval  | period |
|controller | add or remove sources |
|sweep      | period level slope holdOff sourceIn channelNumIn freeRun |
|program    | userAddedParameter |
|composer   | none in this base class |
|fd         | none in this base class |
|source      | none in this base class |



### Language Bindings

I'm not sure if the shell comes before the language bindings or the
language bindings come before the shell, or they may just have to both
come at the same time.  I like ruby to start with.




### Why Quickscope is written in C:

We write Quickscope is C for many reasons.

 1. We the original writers of Quickscope have lots of experience in
writing C code.

 2. Making bindings to others languages is easiest if Quickscope is written
in C, or so we imagine.

 3. The compile time is much faster than if it where written in C++ or D.

 4. It's less lines of code than if it's written in C++.  Debatable, yes.

 5. X11 and GTK+ are written in C and have good C interfaces.

 6. Less dependencies, which is a major plus.

 7. Easier access to input devices given the operating system is written
in C.

 8. None of these reasons are overwhelming, C++ would be okay too.



#### Legal Notice

  >Quickscope - a software oscilloscope

  >Copyright (C) 2012-2015  Lance Arsenault

This file is part of Quickscope.

Quickscope is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the Free
Software Foundation, either version 3 of the License, or (at your option)
any later version.

Quickscope is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License, in
the file COPYING, for more details.  

You should have received a copy of the GNU General Public License along
with Quickscope.  If not, see http://www.gnu.org/licenses/.

