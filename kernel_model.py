# Not a runnable code. Just a algorithm on python

EXIT_TAG = 0
FORK_TAG = 1
PIPE_TAG = 2
MMAP_TAG = 3
MRD_TAG = 4
MWR_TAG = 5

class MMU :
    def ___init___() : pass
    def table(pid) : pass
    def make_empty_table(pid) : pass
    def make_copied_table(pid, ppid) : pass
    def remove_table(pid) : pass 
    
class MMUTableEntry :
    def mapped(ptr) : pass
    def map(fd, length, offset) : pass
    def read(ptr) : pass
    def write(ptr, value) : pass

mmu = MMU()

def new_pid() : pass
def term_pid(pid) : pass

class Process :
    def __init__(self, func, syscall, next)
        self.syscall = syscall
        self.next = next
        self.func = func
    
    def run(self, args)
        res = self.func(args)
        return (next, syscall, res)

class Pipe :
    def ___init___(self, pipes)
        pipes.push(self)
        self.q = []

    def inf(self) :
        return PipeReader(self)
        
    def outf(self) :
        return PipeWriter(self)		

class PipeReader :
    def ___init___(self, pipe)
        self.pipe = pipe
        self.canRead = True
        self.canWrite = False
	
    def read(self) :
        return self.pipe.q.pull() 
	
	
class PipeWriter :
    def ___init___(self, Pipe pipe)
        self.pipe = pipe
        self.canRead = False
        self.canWrite = True
	
    def write(self, t) :
        return self.pipe.q.push(t) 

def kernel(main, args):
    pipes = []
    mainpid = new_pid()
    mmu.make_empty_table(mainpid)
    processes = [(main, mainpid, [stdin, stdout, stderr], args)]
    while processes:
        (prog, pid, fdtable, args) = processes.pop()
        (next, syscall, sargs) = prog.run(args)
        if syscall == EXIT_TAG:
            excode = sargs[0]
            fdtable.clear()
            mmu.remove_table(pid)
            term_pid(pid)
            if excode == 0: 
                print("Execution done")
            else 
                print("Execution failed with {} code".format(excode))
        elif syscall == FORK_TAG:
            newpid = new_pid()
            mmu.make_cloned_table(new_pid, pid)
            processes.push((next, pid, fdtable, [0]))
            processes.push((next, newpid, fdtable.clone(), [pid]))
        elif syscall == PIPE_TAG:
            pipe = Pipe(pipes)
            fdtable.push(pipe.inf)
            fdtable.push(pipe.outf)
            processes.push((next, pid, fdtable, [fdtable.size() - 2, fdtable.size() - 1]))
        elif syscall == MRD_TAG:
            ptr = sargs[0]
            mtable = mmu.table(pid)
            if mtable.mapped(ptr):
                res = [0, mtable.get(ptr)]
            else
                res = [-1]    
            processes.append((next, pid, fdtable, res))
        elif syscall == MWR_TAG:
            ptr = sargs[0] 
            value = sargs[1] 
            mtable = mmu.table(pid)
            if mtable.mapped(ptr):
                mtable.write(ptr, value)
                res = 0
            else
                res = -1 
            processes.append((next, pid, fdtable, [res]))
        elif syscall == MMAP_TAG:
            fd = sargs[0]
            length = sargs[1]
            offset = sargs[2]
            mtable = mmu.table(pid) 
            ptr = mtable.map(fd, length, offset)
            processes.append((next, pid, fdtable, [ptr]))           
        else:
            print("ERROR: No such syscall")
            processes.append(next, pid, fdtable, [])
    pipes.clear()    
    print("Kernel has run all processes")
