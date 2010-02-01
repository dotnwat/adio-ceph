#!/usr/bin/env perl
#
# Copyright (c) 2010 Cisco Systems, Inc.
#
# Script to generate PHP-ized files of Open MPI tarball-installed man
# pages.
#

use strict;
use File::Find;
use File::Basename;
use Cwd;

my $mandir;
my $version;

# Read command line arguments
while (@ARGV) {
    my $a = $ARGV[0];
    if ($a eq "--mandir" && $#ARGV >= 1) {
        shift @ARGV;
        $mandir = $ARGV[0];
        print "Found mandir: $mandir\n";
        shift @ARGV;
    }

    elsif ($a eq "--version" && $#ARGV >= 1) {
        shift @ARGV;
        $version = $ARGV[0];
        print "Found version: $version\n";
        shift @ARGV;
    }
}

# Check that we have what we need
if (!defined($mandir) || !defined($version)) {
    print "Usage: $0 --mandir dir --version version\n";
    exit(1);
}

# Setup
my @files;
my $pwd = Cwd::cwd();

# Find all *.[0-9] files in the $mandir tree.
&File::Find::find(
    sub {
        push(@files, $File::Find::name) if (-f $_ && $_ =~ /\.[1-9]$/);
    }, $mandir);

# Must cd into the $mandir directory because some of the man pages
# refer to other man pages by "man/<filename>" relative path names.
chdir($mandir);

my %dirs;
my $outfiles;

# Generate a PHP file for each man page.
foreach my $file (@files) {
    my $b = basename($file);
    $b =~ m/(.*)\.([0-9])$/;

    my $name = $1;
    my $section = $2;

    my $outfile = "$pwd/man$section/$b.php";
    my $outdir = dirname($outfile);
    $dirs{$outdir} = "";
    push(@{$outfiles->{$section}}, {
        name => $name,
        file => "man$section/$b.php",
         });

    mkdir($outdir)
        if (! -d $outdir);

    print "*** Generating: $name ($section)\n";

    # Run the groff command and send the output to the file
#    open(CMD, "groff -mandoc -T html $file|") || die("Can't open command");
    open(CMD, "man $file | man2html -bare -botm 4 -topm 4|") || die("Can't open command");
    my $text;
    $text .= $_
        while (<CMD>);
    close(CMD);

    # Post-process the text:
    # Remove <head> ... </head>
    # Remove <!doctype ...>
    # Remove <meta ...>
    # Remove <style ...> ... </style>
    # Remove <title> ... </title>
    # Remove <html> and </html>
    # Remove <body> and </body>

    $text =~ s/<head>.*<\/head>//is;
    $text =~ s/<!doctype.*?>//is;
    $text =~ s/<html>//i;
    $text =~ s/<\/html>//i;
    $text =~ s/<body>//i;
    $text =~ s/<\/body>//i;

    # Remove carriage returns, extra white space, and double blank
    # lines
    $text =~ s/\r//g;
    $text =~ s/[ \t]+\n/\n/g;
    $text =~ s/\n{3,}/\n\n/g;

    # Cross-link to any other man pages that we might have.  Search
    # through the string for MPI_<foo> and look for any corresponding
    # man pages in @files.  Sequentially replace MPI_<foo> with
    # $replaced<foo> so that we can find all the MPI_<foo>'s (we'll
    # put the "MPI_" back when we're ).
    my $replace = "ZAZZZERZAZ_";

    # This is a doozy of a regexp (see perlre(1)).  Look for MPI_<foo>
    # cases that aren't followed by .[0-9].php (i.e., not the href
    # clause of an A HTML tag).
    while ($text =~ m/^(.*\W)MPI_(\w+(?!\.[0-9]\.php))(\W.*)$/s) {
        my $comp = lc("mpi_$2");
#        print "Found: $2 -- looking for $comp: ";

        my $prefix = $1;
        my $meat = $2;
        my $suffix = $3;

        my $replaced = 0;
        foreach my $f2 (@files) {
            # If we have another file that matches the regexp that we
            # just pulled out from the text, *and* that file is not
            # the same file that we're already processing (i.e., don't
            # link to myself!), then link to it.
            if (basename(lc($f2)) =~ /^$comp\.[0-9]/ && $f2 ne $file) {
                # Hard-coded to link only to MPI API functions in
                # section 3 (i.e., ../man3/<foo>).
                my $link_file = "../man3/" . basename($f2) . ".php";
#                print "Linked to $link_file!\n";
                $text = "$prefix<a href=\"$link_file\">$replace$meat</a>$suffix";
                $replaced = 1;
                last;
            }
        }
        if (!$replaced) {
#            print "Not linked\n";
            $text = "$prefix$replace$meat$suffix";
        }
    }
    # Now replace the $replaced back with MPI_.
    $text =~ s/$replace/MPI_/g;

    # Obscure any email addresses in there; don't want to give the
    # spammers any new fodder!
    $text =~ s/(\W)[\w\.\-]+@[\w.\-]+(\W)/$1email-address-removed$2/g;

    # Setup meta name: make the MPI name be all caps if we're in
    # section 3 and it has an MPI_ prefix.
    my $meta_name = $name;
    if (3 == $section && $name =~ /^MPI_/) {
        $meta_name = uc($name);
    }
    
    # Now we're left with what we want.  Output the PHP page.
    # Write the output PHP file with our own header and footer,
    # suitable for www.open-mpi.org.
    unlink($outfile);
    open(FILE, ">$outfile") || die "Can't open $outfile";
    print FILE '<?php
$topdir = "../../..";
$title = "' . "$name($section) man page (version $version)" . '";
$meta_desc = "Open MPI v' . "$version man page: $meta_name($section)" . '";

include_once("$topdir/doc/nav.inc");
include_once("$topdir/includes/header.inc");
?>
<p> <a href="../">&laquo; Return to documentation listing</a></p>
' . $text . '
<p> <a href="../">&laquo; Return to documentation listing</a></p>
<?php
include_once("$topdir/includes/footer.inc");
';
    close(FILE);
}

# Now write out an index.php file in each subdir
foreach my $dir (keys(%dirs)) {
    print "Writing index file in $dir...\n";
    open(FILE, ">$dir/index.php") || die "Can't open $dir/index.php";
    print FILE '<?php header("Location: ..");
';
    close(FILE);
}

# Now write out a top-level data-<version>.inc file
my $file = "$pwd/data-$version.inc";
print "Writing $file...\n";
open(FILE, ">$file") || die "Can't open $file";
print FILE '<?php
$version = "' . $version . '";

';

foreach my $section (sort(keys(%{$outfiles}))) {
    print FILE '$files[] = "' . $section . '";
$files_' . $section . ' = array(';
    my $first = 1;
    my @f;
    # The hash isn't sorted, so build up an array of just the
    # filenames
    foreach my $file (@{$outfiles->{$section}}) {
        push(@f, $file->{name});
    }
    # Now output the sorted filenames
    foreach my $file (sort(@f)) {
        print FILE ", "
            if (!$first);
        $first = 0;
        print FILE '"' . $file . '"';
    }
    print FILE ");\n\n";
}
close(FILE);

# Print the top-level engine file for this version (it will use the
# data-<version>.inc file).
open(FILE, ">index.php") || die "Can't open index.php";
print FILE '<?php 
$topdir = "../..";
include_once("data-' . $version . '.inc");
include_once("../engine.inc");
';
close(FILE);

