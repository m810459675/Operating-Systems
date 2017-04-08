

<h1> RAMDISK</h1>
<!DOCTYPE html PUBLIC "-//IETF//DTD HTML//EN">
<html class="gr__courses_ncsu_edu"><head><meta http-equiv="Content-Type" content="text/html; charset=windows-1252">
</head>

<body data-gr-c-s-loaded="true">
<div align="center" style="font-family:Trebuchet MS">
<div align="left" style="width: 700px;">

<hr>
<h3>Assignment:</h3>
<p>
Implement an in-memory filesystem (ie, RAMDISK) using FUSE.
</p>

<hr>
<h3>Description</h3>

<p>
Design,implement, and evaluate an
<i>in-memory filesystem</i>.
</p>
<p>
</p>

<h4>FUSE</h4>

Modern operating systems support multiple filesystems.
The operating system directs each filesystem operation to the
appropriate implementation of the routine.
It multiplexes the filesystem calls across many distinct
implementations.
For example, on a read system call the operation system uses NFS code
if it is an NFS file but uses ext3 code if it is an ext3 file.
<p>
<a href="https://github.com/libfuse/libfuse">FUSE</a>
(Filesystem in Userspace) is an interface that exports filesystem
operations to user-space.
Thus filesystem routines are executed in user-space.
Continuing the example, if the file is a FUSE file then operating
system upcalls into user space in order to invoke the code associated
with read.

</p><h3>RAMDISK</h3>

<p>
Students will create an <i>in-memory filesystem</i>.
Instead of reading and writing disk blocks, the RAMDISK filesystem
will use main memory for storage.
(Note: When the main memory is over allocated, the operating system will page
some memory out to disk.
For this assignment, we still consider that in-memory.)
</p>
<p>
The RAMDISK filesystem  will support the basic POSIX/Unix commands
listed below.
Externally, the RAMDISK appears as a standard Unix FS.
Notably, it is hierarchtical (has directories) and must support
standard accesses, such as read, write, and append. 
However, the filesystem is not persistent.
The data and metadata are lost when the process terminates, which is
also when the process frees the memory it has allocated.
</p>
<p>
The internal design of the filesystem is left to the student.
For example, the Unix inode structure does not have to be mimicked; in
fact, you need not have inodes at all.
When RAMDISK is started it will setup the filesystem, including all
metadata.
In other words, there is no need to run <i>mkfs(8)</i>.
</p>
<p>
RAMDISK should not write any data to disk.
</p>

<h4>Basics for RAMDISK</h4>

Be able to run
<i>postmark</i></a>
in <b>unbuffered</b> mode.
This requires supporting at least the following
system calls.
<ul>
  <li>open, close
  </li><li>read, write
  </li><li>creat [sic], mkdir
  </li><li>unlink, rmdir
  </li><li>opendir, readdir
</li></ul>

<h4>Limitations for RAMDISK</h4>

RAMDISK is <b>not</b> expected to support the following:
<ul>
  <li>Access control,</li>
  <li>Links, and </li>
  <li>Symbolic links.</li>
</ul>


 
</div></div></body></html>
