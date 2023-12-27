
"""*******************************************
	python code for project 1 in
	CSE 107 in 2022 winter, UC Santa Cruz,
			for Prof. Chen Qian.
**********************************************
	Student name:
	UCSC email: arajan2@ucsc.edu
"""

import numpy as np
import matplotlib.pyplot as plt
import simplerandom.iterators as sri

"""
Make a scatter plot of the distribution for these three RNG.
You'll generate num = 10,000 random number in range [0, num).
Make a single scatter plot using matplotlib with the x-axis being 
index of number and the y-axis being the number.

Hint(s):
    1. You'll call plt.scatter(...) for each rng. 
    Make sure your calls are of the form:
    'plt.scatter(x_vals, y_vals, c = 'b', s=2)' where c = 'b' indicates
    blue and s = 2 is to set the size of points. You may want to use
    "r", "g", and "b", a different color for each rng.
    2. Use plt.savefig(...).
"""

def distribution_random():
    np.random.seed(0)

    rand_nums = []
    index_nums = []
    
    for i in range(10000):
        rand_nums.append(np.random.randint(0, 10000))
        index_nums.append(i)

    plt.scatter(index_nums, rand_nums, c = 'b', s = 2)
    # syntax for labels: 
    # https://matplotlib.org/3.1.1/api/_as_gen/matplotlib.pyplot.title.html
    # https://matplotlib.org/3.1.1/api/_as_gen/matplotlib.pyplot.xlabel.html
    plt.title("np.random")
    plt.xlabel("index")
    plt.ylabel("random number")
    plt.savefig("np.random.png")
    plt.show()
    
    pass

def distribution_KISS():
    rng_kiss = sri.KISS(123958, 34987243, 3495825239, 2398172431)

    rand_nums = []
    index_nums = []

    for i in range(10000):
        rand_nums.append(next(rng_kiss))
        index_nums.append(i)
    
    plt.scatter(index_nums, rand_nums, c = 'r', s = 2)
    # syntax for labels: 
    # https://matplotlib.org/3.1.1/api/_as_gen/matplotlib.pyplot.title.html
    # https://matplotlib.org/3.1.1/api/_as_gen/matplotlib.pyplot.xlabel.html
    plt.title("KISS Distribution")
    plt.xlabel("index")
    plt.ylabel("random number")
    plt.savefig("KISS Distribution.png")
    plt.show()
    
    pass

def distribution_SHR3():
    rng_shr3 = sri.SHR3(3360276411)

    rand_nums = []
    index_nums = []
    
    for i in range(10000):
        rand_nums.append(next(rng_shr3))
        index_nums.append(i)
    
    plt.scatter(index_nums, rand_nums, c = 'g', s = 2)
    # syntax for labels: 
    # https://matplotlib.org/3.1.1/api/_as_gen/matplotlib.pyplot.title.html
    # https://matplotlib.org/3.1.1/api/_as_gen/matplotlib.pyplot.xlabel.html
    plt.title("SHR3 Distribution")
    plt.xlabel("index")
    plt.ylabel("random number")
    plt.savefig("SHR3 Distribution.png")
    plt.show()
    
    pass

def pingpong(n:int=21, p:float=0.3, ntrials:int=5000, seed:int=0):
    """
    Write code to simulate a ping pong game to n points,
    where the probability of you winning a single point is p.
    You must win by 2; for example, if the score is 21 - 20, 
    the game isn't over yet. Simulate ntrials # of games.
    :param n: The number of points to play to.
    :param p: The probability of YOU winning a single point.
    :param ntrials: The number of trials to simulate.
    :return: returns the probability YOU win the overall game.
    You can ONLY use the function np.random.random() to generate randomness; 
    this function generates a random float from the interval [0, 1).
    """

    def sim_one_game():
        #     """
        #     This is a nested function only accessible by parent sim_prob,
        #     which we're in now. You may want to implement this function!
        #     """
        # TODO: Your code here (10-20 lines)
        
        player_one = 0
        player_two = 0
        
        while ((player_one < n and player_two < n) or (((player_one >= n) or (player_two >= n)) and abs(player_one - player_two) < 2)):
            q = np.random.random()
            if (q < p):
                player_one += 1
            else:
                player_two += 1
                
        if (player_one > player_two):
            return 1
        else:
            return 0


    win = 0
    win_prob = 0
    np.random.seed(5)

    
    for i in range(ntrials):
        if (sim_one_game()):
            win += 1

    win_prob = win/ntrials
    
    return win_prob
        
def plot_output():
    """
    Make a single plot using matplotlib with the x-axis being p
    for different values of p in {0, 0.04, 0.08,...,0.96, 1.0} 
    and the y-axis being the probability of winning the overall game 
    (use your previous function). Plot 3 "curves" in different colors, 
    one for each n in {3,11,21}.
    You can code up your solution here. Make sure to label your axes
    and title your plot appropriately, as well as include a 
    legend!
    Hint(s):
    1. You'll call plt.plot(...) 3 times total, one for each
    n. Make sure your calls are of the form:
    'plt.plot(x_vals, y_vals, "-b", label="n=11")' where "-b" indicates
    blue and "n=11" is to say these set of points is for n=11. You may 
    want to use "-r", "-g", and "-b", a different color for each n.
    2. Use plt.legend(loc="upper left").
    3. Use plt.savefig(...).
    :return: Nothing. Just save the plot you made!
    """
    
    pass # TODO: Your code here (10-20 lines)

    probs_3 = []
    three = 0

    probs_11 = []
    eleven = 0

    probs_21 = []
    twentyone = 0

    p = [0, 0.04, 0.08, 0.12, 0.16, 0.20, 0.24, 0.28, 0.32, 0.36, 0.4, 0.44, 0.48, 0.52, 0.56, 0.6, 0.64, 0.68, 0.72, 0.76, 0.8, 0.84, 0.88, 0.92, 0.96, 1.0]
    
    for i in p:
        three = pingpong(3, i)
        probs_3.append(three)
    for i in p:
        eleven = pingpong(11, i)
        probs_11.append(eleven)
    for i in p:
        twentyone = pingpong(21, i)
        probs_21.append(twentyone)
        

    # plots
    plt.plot(p, probs_3, "-r", label = "n=3")
    plt.plot(p, probs_11, "-b", label = "n=11")
    plt.plot(p, probs_21, "-g", label = "n=21")

    #legends/labels
    # syntax for labels: 
    # https://matplotlib.org/3.1.1/api/_as_gen/matplotlib.pyplot.title.html
    # https://matplotlib.org/3.1.1/api/_as_gen/matplotlib.pyplot.xlabel.html
    plt.title("Relating P(win point) to P(win game)")
    plt.xlabel("P (win point)")
    plt.ylabel("P (win game)")
    plt.legend()
    plt.savefig("Relating P(win point) to P(win game).png")
    
    plt.show()


if __name__ == '__main__':
    # You can test out things here. Feel free to write anything below.
    #print("hello world")
    distribution_random()
    distribution_KISS()
    distribution_SHR3()

    plot_output()


