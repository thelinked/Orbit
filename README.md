Orbit
=====

About
-----
A C++ library for task and data parallelism.
Based on a series of blog posts by Stefan Reinalter of Molecular Matters.
http://molecularmusings.wordpress.com/tag/scheduler/

Example
-------
	using namespace orbit;
    Scheduler scheduler;
    scheduler.initialise(4);

    auto task = scheduler.addAndRunTask(nullptr, 
    	[](){std::cout << "HelloWorld";});
    scheduler.wait(task);


License
------------
Copyright 2013 Daniel Hartnett

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.