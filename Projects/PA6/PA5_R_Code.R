w = c(100, 200, 300, 400, 500, 600, 700, 800)
run_w = c(15.245, 16.15, 16.47, 16.845, 16.76, 17.205, 17.385, 17.73407)

plot(w, run_w, xlab="Worker Channels", ylab="Runtime (s)", main="Worker channels vs. Runtimes")

b = c(25, 50, 75, 100, 125, 150, 175, 200)
run_b = c(17.18, 16.8, 16.94, 16.53, 17.41, 17.55, 17.14, 16.82)

plot(b, run_b, xlab="Bounded Buffer Size", ylab="Runtime (s)", main="Bounded Buffer Size vs. Runtime")
