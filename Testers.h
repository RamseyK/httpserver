//
//  Testers.h
//  httpserver
//
//  Created by Ramsey Kant on 10/25/11.
//  Copyright (c) 2011 __MyCompanyName__. All rights reserved.
//

#ifndef _TESTERS_H_
#define _TESTERS_H_

#include "ResourceManager.h"

// Function Declarations
void testAll();
void testResManDisk();
void testResManMem();

void testAll() {
    printf("Testing all subcomponents of the program!\n");
    testResManDisk();
    testResManMem();
}

// Test the resource manager for the local hard disk
void testResManDisk() {
    bool pass = true;
    
    printf("Testing: Resource Manager, Local Hard Disk\n");
    
    ResourceManager *resMan = new ResourceManager("./resources", false);
    
    // Try listing the top level /resources directory
    std::string listing = resMan->listDirectory("/");
    if(listing.size() == 0) {
        printf("Could not retrive listing, invalid directory\n");
        pass = false;
    } else {
        printf("Listing:\n%s\n", listing.c_str());
    }
    
    // Try listing an unknown directory, this should fail
    printf("Retriving a listing of an invalid directory. This should fail:\n");
    listing = resMan->listDirectory("/bullshit12345");
    if(listing.size() == 0) {
        printf("Could not retrive listing, invalid directory\n");
    } else {
        printf("Listing:\n%s\n", listing.c_str());
        pass = false;
    }
    
    // Get a resource that should exist (test.txt in /resources)
    Resource *res1 = resMan->getResource("/test.txt");
    if(res1 != NULL) {
        printf("test.txt -> Expected resource found!\n");
    } else {
        printf("ERROR: Expected resource not found!\n");
        pass = false;
    }
    
    if(pass)
        printf("Resource Manager Local hard Disk test PASSED!\n");
    else
        printf("Resource Manager Local hard Disk test FAILED!\n");
    
    printf("\n");
    
    delete resMan;
}

// Test the resource manager in memory FS
void testResManMem() {
    bool pass = true;
    
    printf("Testing: Resource Manager, Memory FS\n");
    
    ResourceManager *resMan = new ResourceManager("", true);
    
    // Get a known-to-exist resource
    Resource *res1 = resMan->getResource("/hey/test2.mres");
    if(res1 != NULL) {
        printf("/hey/test -> Expected resource found!\n");
    } else {
        printf("ERROR: Expected resource not found!\n");
        pass = false;
    }
    
    // List top level in memory
    std::string listing = resMan->listDirectory("/");
    if(listing.size() == 0) {
        printf("Could not retrive listing, invalid or empty directory\n");
        pass = false;
    } else {
        printf("Listing:\n%s\n", listing.c_str());
    }
    
    // List invalid directory in memory, this should fail
    printf("Retriving a listing of an invalid directory. This should fail:\n");
    listing = resMan->listDirectory("/hey/bullshit123");
    if(listing.size() == 0) {
        printf("Could not retrive listing, invalid or empty directory\n");
    } else {
        printf("Listing:\n%s\n", listing.c_str());
        pass = false;
    }
    
    if(pass)
        printf("Resource Manager Memory FS test PASSED!\n");
    else
        printf("Resource Manager Memory FS test FAILED!\n");
    
    printf("\n");
    
    delete resMan;
}

#endif
