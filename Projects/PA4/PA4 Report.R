w = c(500, 600, 700, 800, 900, 1000)
t_w = c(21.77, 19.35, 18.49, 20.82, 18.22, 20.16)

rm(list=ls())

b = c(100, 120, 140, 160, 180, 200)
t_b = c(19.82, 18.91, 17.55, 20.40, 25.83, 17.81)

plot(w, t_w, main="Worker Threads v. Execution Time", xlab="# of Worker Threads", ylab="Execution Time(s)")
plot(b, t_b, main="Request Buffer Size v. Execution Time", xlab="Request Buffer Size(Bytes)", ylab="Execution Time(s)")

t_w_f = c(78.32, 76.6, 81.25, 79.11, 78.18, 81.86)
m = c(200, 300, 400, 500, 600, 700)
m_t = c(94.25, 66.78, 50.87, 43.19, 36.3, 31.34)

plot(w, t_w_f, main="Worker Threads v. Execution Time", xlab="# of Worker Threads", ylab="Execution Time(s)")
plot(m, m_t, main="Buffer Capacity v. Execution Time", xlab="Buffer Capacity (Bytes)", ylab="Execution Time(s)")
