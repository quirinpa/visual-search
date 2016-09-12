> Internship at MIT-I (Madeira)

# Introduction
At first it was proposed that i would create a framework based on OpenCV for matching an object with images in an array or a folder. I found myself with the job of selecting the algorithms that performed better in terms of speed or that held better results in situations where the object to be found was farther away, occluded, rotated, or blurred. A binary conclusion had to be then reached programmatically in regards to the existence of an object in the target scene.  

To the effect of realising this binary conclusion, matches that do not actually correspond to the object should be filtered out to the maximum extent that does not compromise performance too much. There are many such filtering techniques:
- David Lowe's ratio test
- Checking correspondences in both directions or *cross-matching*.
- Various kinds of Geometric tests

This project uses a *subspace-clustering* technique that helps in the removal of geometrically isolated small groups of matches, in addition to a custom-made *cross-matching* algorithm. David-Lowe's ratio test is allowed through a compiler option.

Some issues are involved with the concept of creating a framework around OpenCV:
- It would not be compatible with Python and other Languages, as OpenCV is, unless i ported it, and i don't have time for it during the internship.
- If the user had to link to OpenCV's libraries (which takes a lot of time), why would we take away its functionality by abstracting it away?
- Feature and descriptor extraction is slow, and it needs to be performed only once per dataset, but to force cache extracted features and descriptors in a file seems like a poor "quality" for a library.

The well-defined purpose of this project seemed to be a job for a limited set of command-line tools rather than for a framework built on top of another framework, so i suggested that i could build that instead, and my mentor agreed. Now the desired functionalities are available in whichever environment possesses POSIX-compliant system calls.

# Installation
Pre-compiled binaries are not available, so you have to clone the project's repository, install the OpenCV library and compile the project yourself. Like so:
```sh
git clone https://github.com/quirinpa/visual-search
cd visual-search
make
sudo make install
```
# Usage
Two tools were developed for this project. One (*vsconvert*) generates datasets, the other (*vsmatch*) matches datasets with one another. As i mentioned previously, this is so that we don't need to run the processor-heavy extractors and detectors each time we need to match datasets.

Their documentation is available through either "**tool** --help" or "man **tool**".

## vsconvert
To generate a dataset of train images using the default (*BRISK*) extractor and detector algorithms over all the files in the /example\_images/train, for example, one should run:
```sh
vsconvert -o train_dataset /starbucks/train/*
```
Then, generate your query dataset (only one image allowed), through:
```sh
vsconvert -qo query_dataset /starbucks/query.png
```

## vsmatch
Establish whether or not the image from which the *query_dataset* was created matches any of the images that originated *train_dataset*, and print out the path of those images (from where *vsconvert* was run). You should see something like:
```sh
```
# Future developments
As with any modest project, there is room for improvement. These are some of the ways in which this project might be enhanced in the future.

## Bugs
### AKAZE
Unfortunately, as i am forced to write this paper, i've had no time to fix a problem using this detector and extractor type. I intend to fix this soon.
### EXPERIMENTAL\_FLANN
Only works with binary descriptors
### KNN\_MATCH
Some of the features are still in development stage, AKAZE still causes segmentation
The EXPERIMENTAL
## Standards
As a means of enhancing my programming workflow, i've started studying the Linux System and and its tools about 2,5 years ago. I've found that i really liked the Linux philosophy of making software that solves one problem well. I try to make code that makes sense to me, and that will keep on making sense if i read it in the future. Sometimes, when i'm on a schedule, i tend to worry more about functionality than readability, but i eventually improve upon it and when i do that i apply my latest knowledge collected from sources like [gnu.org/prep/standards/standards.html](https://www.gnu.org/prep/stardards/standards.html). I will surely come back to this project and improve upon it further when i'm more knowledgeable. Besides, the git repository is online at [http://github.com/quirinpa/visual-search](http://github.com/quirinpa/visual-search) and is open for your contribution.

## Features
# The tools
## cvconvert
> create a dataset for feature-based image recognition.


This is a tool
It would also limit the user's programming language to C++ 
After realising the slow compile times associated with OpenCV, aspiring to develop efficient, effective tools that adhere to the Linux philosophy, i proposed that instead of a framework, i could develop POSIX-compliant command-line tools.
my knowledge, this cannot be done with 100% accuracy, 
Seen as this was the well-defined purpose of my project, after experimenting with the abstraction of OpenCV methods, and realisingi proposed to my mentor that i made a couple of command-line tools instead. *cvconvert* to extract image features and keypoint descriptors and cache them in a file, another to match a query file
I've researched OpenCV Scale-Invariant-Feature-Transform and other algorithms like ORB, BRISK and SURF. Experimented with the various types of matchers and filters that gave me differing results
Seen as my job was to create a framework that abstracts away some of the workings of another framework, and because the kind of functionality desired was well-defined, i considered creating POSIX-compliant command-line tools that can be used to the effect of the proposed features, and that's what i ended up doing.

i researched my options and experimented with various techniques provided by OpenCV
