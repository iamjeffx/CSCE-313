w = c(100, 200, 300, 400, 500, 600, 700, 800)
wt = c(153.55, 101.9, 105.55, 123.28, 107.51, 132.39, 127.87, 220.21)
plot(w, wt, xlab="Workers", ylab="Runtime(s)", main="Workers vs. Runtime")

b = c(25, 50, 75, 100, 125, 150, 175, 200)
bt = c(120.46, 113.1, 97.46, 91.91, 103.63, 120.58, 144.88, 135.86)
plot(b, bt, xlab="Buffer Size", ylab="Runtime(s)", main="Buffer Size vs. Runtime")
