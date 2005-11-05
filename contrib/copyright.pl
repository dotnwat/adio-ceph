#!/usr/bin/env perl
#
# Copyright (c) 2004-2005 The Trustees of Indiana University and Indiana
#                         University Research and Technology
#                         Corporation.  All rights reserved.
# Copyright (c) 2004-2005 The University of Tennessee and The University
#                         of Tennessee Research Foundation.  All rights
#                         reserved.
# Copyright (c) 2004-2005 High Performance Computing Center Stuttgart, 
#                         University of Stuttgart.  All rights reserved.
# Copyright (c) 2004-2005 The Regents of the University of California.
#                         All rights reserved.
# $COPYRIGHT$
# 
# Additional copyrights may follow
# 
# $HEADER$
#
# Utility to stamp copyrights into files.  This is only being checked
# into SVN because we have LANL and HLRS copyrights pending, and this
# script will be helpful in adding them to the tree (I'm sure the
# script will need to be modified, but it's got all the relevant
# File::Find code, etc.).

use strict;
use File::Find;
use Cwd;

my $verbose = 0;
my %files_found;
my @skip_dirs = ( "doxygen", ".svn", ".deps", ".libs", "libltdl", "autom4te.cache" );

my $iu = "Copyright (c) 2004-2005 The Trustees of Indiana University and Indiana
                        University Research and Technology
                        Corporation.  All rights reserved.";
my $utk = "Copyright (c) 2004-2005 The University of Tennessee and The University
                        of Tennessee Research Foundation.  All rights
                        reserved.";
my $osu = "Copyright (c) 2004 The Ohio State University
                        All rights reserved.";
my $hlrs = "Copyright (c) 2004-2005 High Performance Computing Center Stuttgart, 
                        University of Stuttgart.  All rights reserved.";
my $uc = "Copyright (c) 2004-2005 The Regents of the University of California.
                        All rights reserved.";

my $copy = "$uc\n\$COPYRIGHT\$";

sub wanted {
    # Setup

    my $file = $_;
    my @dirnames = split('/', $File::Find::dir);
    my $dir = $dirnames[$#dirnames];

    # Is the file a dir?

    my $is_dir = (-d $file);

    # Do we want this dir?

    for (my $i = 0; $i <= $#skip_dirs; ++$i) {
        if ($skip_dirs[$i] eq $dir || 
            ($is_dir && $skip_dirs[$i] eq $file)) {
            print("Skipping dir: $File::Find::dir / $file\n")
                if ($verbose);
            $File::Find::prune = 1;
            return 0;
        }
    }

    # Do we want this file?

    if ($is_dir) {
        print ("Found dir: $File::Find::dir/$file\n")
            if ($verbose);
    } else {
        $files_found{$File::Find::name} = 1;
        print ("Found file: $File::Find::name\n")
            if ($verbose);
    }
    1;
}



my $new_iu1 = "Copyright (c) 2004-2005 The Trustees of Indiana University and Indiana";
my $new_iu2 = "University Research and Technology";
my $new_iu3 = "Corporation.  All rights reserved.";

my $new_utk1 = "Copyright (c) 2004-2005 The University of Tennessee and The University";
my $new_utk2 = "of Tennessee Research Foundation.  All rights";
my $new_utk3 = "reserved.";


find (\&wanted, ".");
my $counts;
my $max_count = -1;
foreach my $file (sort keys %files_found) {
     open FILE, $file;
    my @found = grep(/\$COPYRIGHT\$/, <FILE>);
    close(FILE);
    if ($#found >= 0) {
        print "Found file: $file\n";
        open FILE, $file;
        my @lines = <FILE>;
        close FILE;

#my $old_iu1 = "Copyright \(c\) 2004-2005 The Trustees of Indiana University\.";
#my $old_iu2 = "All rights reserved.";

#my $old_utk1 = "Copyright (c) 2004-2005 The Trustees of the University of Tennessee.";
#my $old_utk2 = "All rights reserved.";
        
        my $text = join('', @lines);
        $text =~ s/([ \*\!\#\%\/dnl]*)Copyright \(c\) 2004-2005 The Trustees of Indiana University\.\n([ \*\!\#\%\/dnl]*)All rights reserved\.\n/$1$new_iu1\n$2$new_iu2\n$2$new_iu3\n/;
        $text =~ s/([ \*\!\#\%\/dnl]*)Copyright \(c\) 2004-2005 The Trustees of the University of Tennessee\.\n([ \*\!\#\%\/dnl]*)All rights reserved\.\n/$1$new_utk1\n$2$new_utk2\n$2$new_utk3\n/;
        
        open FILENEW, ">$file.new" || die "could not open $file.new\n";
        print FILENEW $text;
        close FILENEW;

        system("cp $file.new $file");
        unlink("$file.new");

        if (0) {
        while (<FILE>) {
            chomp;
            my $line = $_;
            if ($line =~ /\$COPYRIGHT\$/) {
                my $prefix = $line;
                $prefix =~ s/(.+)\$COPYRIGHT\$.*$/\1/;
                if ($prefix ne "\$COPYRIGHT\$") {
                    my $c = $prefix . $copy;
                    $c =~ s/\n/\n$prefix/g;
                    print FILENEW "$c\n";
                } else {
                    print FILENEW "$copy\n";
                }
            } else {
                print FILENEW "$line\n";
            }
        }
        close(FILENEW);
        close(FILE);
        system("cp $file.new $file");
        unlink("$file.new");
    }
    }
}
