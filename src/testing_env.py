from kaggle_environments import make
env = make('chess', debug=True)

result = env.run(["cfish_link.py", "random"])
env.render(mode="ipython", width=1000, height=1000)
