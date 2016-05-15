# Not a runnable code. Just a algorithm on python

EXIT_TAG = 0
FORK_TAG = 1
PIPE_TAG = 2
MMAP_TAG = 3
UNMMAP_TAG = 4

def new_pid() : pass
def term_pid(pid) : pass

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

class VPage :
    def ___init___(self, pType, phPage) 
        self.pType = pType
        self.phPage = phPage   
    
mmu_data = dict()
 
phys_mem = [1024]

def kernel(main, args):
    pipes = []
    mainpid = new_pid()
    mmu_data[mainpid] = dict()
    processes = [(main, mainpid, [stdin, stdout, stderr], args)]
    prevpid = mainpid
    while processes:
        (prog, pid, fdtable, args) = processes.pop()
        if (prevpid != pid):
            restore_mmu(mmu_data[pid])
        (next, syscall, sargs) = prog(args)
        if syscall == EXIT_TAG:
            excode = sargs[0]
            fdtable.clear()
            mmu_data.remove(pid)
            term_pid(pid)
            if excode == 0: 
                print("Execution done")
            else 
                print("Execution failed with {} code".format(excode))
        elif syscall == FORK_TAG:
            newpid = new_pid()
            newvpages = []
            for(vpage : mmu_data[pid]) 
                if vpage.pType == SHARED:
                    newvpages.push(vpage)
            mmu_data[newpid] = newvpages                    
            processes.push((next, pid, fdtable, [newpid]))
            processes.push((next, newpid, fdtable.clone(), [0]))
        elif syscall == PIPE_TAG:
            pipe = Pipe(pipes)
            fdtable.push(pipe.inf)
            fdtable.push(pipe.outf)
            processes.push((next, pid, fdtable, [fdtable.size() - 2, fdtable.size() - 1]))
        elif syscall == MMAP_TAG:
            pages = sargs[0]
            pType = sargs[1]
            prev_key = 0
            found = False
            for (key in mmudata[pid].keys)
                if (key - prev_key > pages) 
                    found = True
                    break
                prev_key = key
            if !found:
               processes.append((next, pid, fdtable, [-1])) 
               continue
            not_allocated = []
            for (i in range 0..len(phys_mem)-1) 
                if (!phys_mem[i].allocated)
                    not_allocated.push[i]
                if (len(not_allocated) == pages)
                    break
            if len(not_allocated) < pages :
                processes.append((next, pid, fdtable, [-1]))
                continue   
            for (i in range 0..pages-1)
                mmu_data[pid][prev_key+i+1](VPage(pType, not_allocated[i]))
                phys_mem[not_allocated[i]].allocated = True    
            processes.append((next, pid, fdtable, [prev_key+1]))     
        elif syscall == UNMMAP_TAG:
            pagenum = sargs[0]
            mmu_data[pid].remove(pagenum)
            processes.append((next, pid, fdtable, []))    
        else:
            print("ERROR: No such syscall")
            processes.append(next, pid, fdtable, [])
        prevpid = pid
    pipes.clear()    
    print("Kernel has run all processes")
