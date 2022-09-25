<head>
<title>Fix-CA</title>
</head>
<body>
<h1>Overview</h1>
<p>
Fix-CA is a <a href="http://www.gimp.org">Gimp</a> plug-in to correct
chromatic aberration (CA).
For general explanation about chromatic aberration, you can find
information from 
<a href="http://en.wikipedia.org/wiki/Chromatic_aberration">Wikipedia</a>
and
<a href="http://www.vanwalree.com/optics/chromatic.html">writings 
by Paul van Walree</a>.  
Fix-CA is able to fix lateral CA caused by lens and colored fringing caused
by light travel through dense material such as glass and water (which is
called directional CA in the program).
</p>

<p>
Current version of Fix-CA is 3.0.2.  The plug-in is written
by Kriang Lerdsuwanakij 
&lt;<a href="mailto:lerdsuwa@users.sourceforge.net">lerdsuwa@users.sourceforge.net</a>&gt; 
and is distributed
under <a href="doc/COPYING">GNU General Public License Version 2</a> (GPLv2).
It comes with absolutely no warranty.  Contact the author for any support
or bug report.
</p>

<h1>Download and building Fix-CA</h1>
<p>
Fix-CA can be downloaded from 
<!--
<a href="http://registry.gimp.org/plugin?id=8668">here</a>.-->
<a href="fix-ca.c">here</a>.
Just get the C source file.
To compile and install the plug-in, you need the development library 
for Gimp.  Usually it's in gimp-devel package in your distribution.
Use the command
<pre>
        gimptool-2.0 --install fix-ca.c
</pre>
will automatically compile and install into the plug-in directory
of your account.  If you wish to make the plug-in available to all
users of the machine, use
<pre>
        gimptool-2.0 --install-admin fix-ca.c
</pre>
instead.  Type
<pre>
        gimptool-2.0 --help
</pre>
for further help.
<p>

<p>
If Gimp is already running in your system, exit and restart Gimp
for the new plug-in to be detected.  The following displays
Fix-CA version 2.1.0 information from Gimp <i>Xtns->Plug-In Browser</i> menu.
</p>
<div align="center">
<img src="img-fix-ca/plug-in-browser.png" border="0" width="760" height="588" />
</div>

<h1>Precompiled binary</h1>
<p>
For Windows, a precompiled binary of Fix-CA is available at <a href="http://photocomix-resources.deviantart.com/art/Fix-Cromatic-Aberration-95683614">here</a>,
courtesy of Francois Collard (compilation) and PhotoComiX-Resources (hosting).
</p>

<h1>Using Fix-CA</h1>
<p>
Fix-CA plug-in can be accessed via the menu <i>Filters->Colors->Chromatic Aberration...</i>.
There are two different modes of chromatic aberration that can be corrected by Fix-CA: 
lateral and directional.
</p>

<h2>Lateral CA</h2>
<p>
Lateral chromatic aberration appears due to lens imperfection. 
For this type of CA, the center of the image has no CA and the CA
gradually increase toward the border.  So the original image, rather than a cropped version,
should be used to eliminate the CA.
</p>

<p>
Below left is the image used to demonstrate Fix-CA plug-in capability 
(<a href="img-fix-ca/full-Wat_Pathum_Wanaram.jpg">full sized original here</a>, 
available under 
<a href="http://www.gnu.org/licenses/fdl.html">GNU Free Documentation License</a> 
Version 1.2 (GFDL) license). It's a photo of Wat Pathum Wanaram,
a buddhist temple next to Siam Paragon, a major shopping mall in Bangkok.
The areas of interest are marked with green rectangles. 
Note that the reduced size here is only for display purpose.  
The actual processing is done on full sized image (with
green rectangles on a different layer, unaffected by the plug-in).
Below right is 200% zoom of the areas.
The chromatic aberration (CA) is visible on the right images as red and
blue lines around bright area.
</p>

<div align="center">
<img src="img-fix-ca/ex-orig.jpg" border="0" width="300" height="450" />
<img src="img-fix-ca/ex-zoom.jpg" border="0" width="486" height="450" />
</div>

<p>
Normally it is not noticable in the final image. But can be quite
visible when the image is cropped to smaller area and then used 
(poor man's zoom).
</p>

<p> Invoking Fix-CA plug-in via the menu <i>Filters->Colors->Chromatic
Aberration...</i>. 
With the settings shown in below left image (for version 2.1.0), 
much of CA can be eliminated.  
For version 3.0.0 blue and red settings depicted are moved 
under the group <i>Lateral</i>.
Below right is the 200% zoom for the resulting change.
</p>

<div align="center">
<img src="img-fix-ca/fix-ca-dialog.png" border="0" width="365" height="455" />
<img src="img-fix-ca/ex-fixed.jpg" border="0" width="486" height="450" />
</div>

<p>
In the dialog box, setting red to -1.5 will shift the red channel inward to
the maximum of 1.5 pixels.
Here positive number means moving outward, while negative number means inward.
Since the orientation of this photo is portrait, the red channel of top and
bottom border will be moved by 1.5 pixels.  
Pixels at the inner part the red channel will be moved less
while the pixels at the center of image will not be moved at all.
(For landscape image, the left and right border will be moved by 1.5 pixels instead).  
The interpolation parameter will control how the plug-in deals with fractional
pixel, for example, if the plug-in decides to move an image pixel by 0.8 pixel,
<i>Linear</i> and <i>Cubic</i> settings will try to get a value by averaging surrounding
pixels while <i>None</i> will pick the nearest pixel (for this example by moving 1 full pixel).
</p>

<p>
The setting <i>Preview saturation</i> changes the saturation for the preview image.
This may help spotting CA problem. The setting does not have any effect on the
final image produced by this filter.
</p>

<p>
In this example we need to trade off between bottom right area (which suffers from
less CA) and the left area (the most CA).
When image is used cropped, you can concentrate on fixing in the part of image
that is actually used and ignore the rest.  A better result can be expected.
Just remember to correct CA before cropping.
</p>

<h2>Directional CA</h2>
<p>
In directional chromatic aberration, the amount of CA is assumed to be the same
throughout the image.  This can happen, for example, when photographing fishes
in an aquarium.  Light travels through dense water and glass and bends differently
depending on the color.  This CA correction mode is introduced in Fix-CA version 3.0.0.
You can specify the amount of shift for blue and red along both X and Y axis.
</p>

<p>
An example is the following photo of a sleeping sea turtle. The left side is the 
complete photo. The right is the turtle head, zoom to 50%. 
CA is severe and noticable at this zoom level.
</p>

<div align="center">
<img src="img-fix-ca/Sea_turtle-orig.jpg" border="0" width="450" height="300" />
<img src="img-fix-ca/Sea_turtle-zoom.jpg" border="0" width="300" height="300" />
</div>

<p>
The amount of CA is roughly the same throughout the image. Starting from
the original, as in the left image below. By adjusting X-axis
CA amount and Y-axis amount to shift color component around, the result
is quite good with the settings on the right.
</p>

<div align="center">
<img src="img-fix-ca/Sea_turtle-dialog_before.png" border="0" width="359" height="684" />
<img src="img-fix-ca/Sea_turtle-dialog.png" border="0" width="359" height="684" />
</div>

<p>
The picture below shows, at 50% zoom, the corrected image of the interested region.
</p>

<div align="center">
<img src="img-fix-ca/Sea_turtle-fixed.jpg" border="0" width="300" height="300" />
</div>

<h1>Version History</h1>
<ul>
<li>3.0.2 (December 22, 2007) Add the missing tile cache that speed up preview.</li>
<li>3.0.1 (July 5, 2007) Fix a bug involving image row cache that cause bad CA
correction when the number of pixel moved is large.</li>
<li>3.0.0 (July 3, 2007) Add CA fix in X and Y axis.</li>
<li>2.1.0 (January 24, 2007) Add saturation in preview. (Suggested by Reiner.) </li>
<li>2.0.0 (December 5, 2006) Improve speed. Add linear and cubic interpolation.</li>
<li>1.0.0 (November 30, 2006) First version.</li>
</ul>

<h1>Future Plan</h1>
<p>
A long target is for the plug-in to work automatically without 
requiring user to carefully choose the amount of shift.
</p>
<p>
Comments are also welcome.
</p>

<div align="right">
<p>
<i>Web page content is copyrighted (c) 2006, 2007 by Kriang Lerdsuwanakij.</i>
</p>
</div>
</body>
