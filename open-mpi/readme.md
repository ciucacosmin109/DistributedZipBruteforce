sudo systemctl start sshd

ssh stud@node2 mkdir -p ~/dad-project/open-mpi && \
scp ~/dad-project/open-mpi/* stud@node2:~/dad-project/open-mpi

mpic++ -o main.elf64 main.c && \
scp ~/dad-project/open-mpi/* stud@node2:~/dad-project/open-mpi && \
mpirun -np 3 -hostfile ./nodefile ./main.elf64 protected.zip

