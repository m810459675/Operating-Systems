<!DOCTYPE html PUBLIC "-//IETF//DTD HTML//EN">
<html><head><meta http-equiv="Content-Type" content="text/html; charset=windows-1252">
<title>MyThread Library</title>
</head>
<body>
<div align="center">
<div align="left" style="font-family:Trebuchet MS;width: 700px;">
<h1>CSC 501 Program <i>Threading library</i></h1>
<hr>
<h3>Assignment:</h3>
Implement a <em>non pre-emptive</em>  user-level threading library.
<hr>

<h3>Description:</h3>
Implement a non pre-emptive user-level threads library
<i>mythread.a</i> with 
the following routines.

<h4>Thread routines.</h4>
<blockquote>
<dl>
  <dt><em>MyThread <strong>MyThreadCreate</strong>
      (void(*start_funct)(void *), void *args)</em>
  </dt><dd>This routine creates a new <em>MyThread</em>.
      The parameter <em>start_func</em> is the function in which the
      new thread starts executing.
      The parameter <em>args</em> is passed to the start function.
      This routine does <em>not</em> pre-empt the invoking thread.
      In others words the parent (invoking) thread will continue to
      run; the child thread will sit in the ready queue.
  </dd><dt><em>void <strong>MyThreadYield</strong>(void)</em>
  </dt><dd>Suspends execution of invoking thread and yield to another thread.
      The invoking thread  remains ready to execute—it is not
      blocked.
      Thus, if there is no other ready thread, the invoking thread will
      continue to execute.
  </dd><dt><em>int <strong>MyThreadJoin</strong>(MyThread thread)</em>
  </dt><dd>Joins the invoking function with the specified child thread.
      If the child has already terminated, do not block.
      Note: A child may have terminated without the parent having
      joined with it.
      Returns 0 on success (after any necessary blocking).
      It returns -1 on failure.
      Failure occurs if specified thread is not an immediate
      child of invoking thread.
  </dd><dt><em>void <strong>MyThreadJoinAll</strong>(void)</em>
  </dt><dd>Waits until all children have terminated.
      Returns immediately if there are no <em>active</em> children.
  </dd><dt><em>void <strong>MyThreadExit</strong>(void)</em>
  </dt><dd>Terminates the invoking thread.  <em><strong>Note:</strong></em>
      all <em>MyThread</em>s are required to invoke this function.
      Do not allow functions to “fall out” of the start
      function.
</dd></dl>
</blockquote>
<h4>Semaphore routines.</h4>
<blockquote>
<dl>
  <dt><em>MySemaphore <strong>MySemaphoreInit</strong>(int initialValue)</em>
  </dt><dd>Create a semaphore.
      Set the initial value to <em>initialValue</em>, which must be
      non-negative.
      A positive initial value has the same effect as invoking
      <em>MySemaphoreSignal</em> the same number of times.
      On error it returns NULL.
  </dd><dt><em>void <strong>MySemaphoreSignal</strong>(MySemaphore sem)</em>
  </dt><dd>Signal semaphore <em>sem</em>.
      The invoking thread is not pre-empted.
  </dd><dt><em>void <strong>MySemaphoreWait</strong>(MySemaphore sem)</em>
  </dt><dd>Wait on semaphore <em>sem</em>.
  </dd><dt><em>int <strong>MySemaphoreDestroy</strong>(MySemaphore sem)</em>
  </dt><dd>Destroy semaphore <em>sem</em>.
      Do not destroy semaphore if any threads are blocked on the
      queue.
      Return 0 on success, -1 on failure.
</dd></dl>
</blockquote>
<h4>Unix process routines.</h4>
The Unix process in which the user-level threads run is not a
<em>MyThread</em>.
Therefore, it will not be placed on the queue of
<em>MyThreads.</em>
Instead it will create the first <em>MyThread</em> and relinquish the
processor to the <em>MyThread</em> engine. 
<p>
The following routine may be executed only by the Unix process.

</p><blockquote>
<dl>      
  <dt><em>void <strong>MyThreadInit</strong>
      (void(*start_funct)(void *), void *args)</em>
  </dt><dd>This routine is called before any other <em>MyThread</em> call.
      It is invoked only by the Unix process.
      It is similar to invoking <em>MyThreadCreate</em> immediately
      followed by  <em>MyThreadJoinAll</em>.
      The <em>MyThread</em> created is the oldest ancestor of all
      <em>MyThread</em>s—it is the
      “main” <em>MyThread</em>.
      This routine can only be invoked once.
      It returns when there are no threads available to run (i.e.,
      the thread ready queue is empty.
</dd></dl>
</blockquote>

</body></html>