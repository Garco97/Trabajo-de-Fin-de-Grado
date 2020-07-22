import os
from os import listdir
from os.path import isfile, join
from collections import defaultdict
from pathlib import Path

import warnings
warnings.simplefilter("ignore")
home = str(Path.home())
os.chdir(home+"/Desktop/diegog-vol3/node/benchmark_comms/")
questions_path = "questions/"
questions = [f for f in listdir(questions_path) if  isfile(join(questions_path, f))]
for question in questions:
    if ".py" in question and "stats.py" != question:
        print(question)
        exec(open(questions_path+"/"+question).read())