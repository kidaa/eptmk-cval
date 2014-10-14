
# Install script

# First make sure the target files exist

unless (-e "src/makefile" && -e "medpost")
{
	print "This program must be run in the install directory.\n";
	exit(1);
}

# Determine the known operating system

if ($ARGV[0])
{
	$os = $ARGV[0];
	shift @ARGV;
}

# Update the compiler

if ($os =~ /solaris/i)
{
	$cc = "CC -w";
}

if ($cc)
{
	print "Using the $cc compiler.\n";

	@x = `cat src/makefile`;
	open F,">src/makefile";
	for (@x)
	{
		chomp;
		s/^CC\=.*/CC=$cc/;
		print F "$_\n";
	}
	close F;
} else
{
	print "Keeping the default compiler.\n";
}

# Make the application

system("cd src; make");

# Edit the medpost script

$idate = gmtime;
$install_dir = `pwd`; chomp $install_dir;

@x = `cat medpost`;
open F,">medpost";
for (@x)
{
	chomp;
	s/^MEDPOST_HOME\=.*/MEDPOST_HOME=$install_dir/;
	s/^INSTALL_DATE\=.*/INSTALL_DATE=\"$idate\"/;
	print F "$_\n";
}
close F;
system("chmod +x medpost");

