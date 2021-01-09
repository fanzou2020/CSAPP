1. 在eval()函数中，检查当前命令是否是内置命令
2. 如果为foreground job，shell等待job的结束
3. 注册SIGCHLD handler, reaping children.

4. bg jobid/pid
5. fg jobid/pid