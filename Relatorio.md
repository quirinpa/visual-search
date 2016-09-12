# Introduction
At first it was proposed that i would create a framework based on OpenCV for matching an object with images in an array or a folder. I found myself with the job of selecting the algorithms that performed better in terms of speed or that held better results in situations where the object to be found was farther away, occluded, rotated, or blurred. A binary conclusion had to be then reached programatically in regards to the existance of an object in the target scene.  

To the effect of realising this binary conclusion, matches that do not actually correspond to the object must be filtered out to the maximum extent possible. There are many filtering techniques for this, for example David Lowe's ratio test, or checking correspondences in both directions ( cross-matching ). However, for the purpose of finding objects that are farther away, these techniques fall short, so i added an additional approach which makes things a little better: subspace-clustering.

Meanwhile, i faced some issues with the concept of creating a framework around OpenCV:
- It would not be compatible with Python and other Languages, as OpenCV is, unless i ported it.
- If the user had to link to OpenCV's libraries, why would we abstract away it's functionalities?
- Feature and Descriptor Extraction is slow, and it only needs to be performed once per dataset.
- OpenCV compile time is very slow.

The well-defined purpose of this project's behaviours, seemed to be a job for a set of command-line tools rather than a framework over a framework, so i suggested to build them instead, and my mentor agreed.

Now the desired functionalities are available in whichever environment posesses Posix-compliant system calls.

# The tools
It would also limit the user's programming language to C++ 
After realising the slow compile times associated with OpenCV, aspiring to develop efficient, effective tools that adhere to the Linux philosophy, i proposed that instead of a framework, i could develop POSIX-compliant command-line tools.
my knowledge, this cannot be done with 100% accuracy, 
Seen as this was the well-defined purpose of my project, after experimenting with the abstraction of OpenCV methods, and realisingi proposed to my mentor that i made a couple of command-line tools instead. *cvconvert* to extract image features and keypoint descriptors and cache them in a file, another to match a query file
I've researched OpenCV Scale-Invariant-Feature-Transform and other algorithms like ORB, BRISK and SURF. Experimented with the various types of matchers and filters that gave me differing results
Seen as my job was to create a framework that abstracts away some of the workings of another framework, and because the kind of functionality desired was well-defined, i considered creating POSIX-compliant command-line tools that can be used to the effect of the proposed features, and that's what i ended up doing.

i researched my options and experimented with various techniques provided by OpenCV
