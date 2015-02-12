#          Copyright Andrey Moshbear 2014-2015.
# Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE_1_0.txt or copy at
#          http://www.boost.org/LICENSE_1_0.txt)
#
# Generator for data file used by mock/http.cc

use warnings;
use strict;
use LWP::UserAgent;

our $instruments_url = 'http://api-sandbox.oanda.com/v1/instruments';
our $instruments_file = 'INSTRUMENTS.json';
our $instruments_hint = 'INSTRUMENTS.valid'; 

our @instruments = undef;

our $rates_url = 'http://api-sandbox.oanda.com/v1/prices?instruments=';
our $rates_file = 'RATES.hx';

sub do_instruments {
	my $_sub = ((caller(0)))[3];
	my $ua = LWP::UserAgent->new();
	my $response = $ua->get($instruments_url);
	die "$_sub: $response->status_line" if !$response->is_success;
	my $data = $response->decoded_content(charset => 'utf-8');
	## instruments json
	open INSTRS, '>'.$instruments_file or die "$_sub: open: $!";
	print INSTRS $data;
	close INSTRS;
	## instruments validation list
	@instruments = sort {$a cmp $b} ($data =~ m/"([^"_]+_[^"_]+)"/g );
	open INSTRS_VALID, '>'.$instruments_hint or die "$_sub: open: $!";
	print INSTRS_VALID join("\n", @instruments);
	close INSTRS_VALID;
}

sub do_rates {
	my $_sub = ((caller(0)))[3];
	my $ua = LWP::UserAgent->new();
	my $response = $ua->get($rates_url.join("%2C", @instruments));
	die "$_sub: $response->status_line" if !$response->is_success;
	## Fancy quasi-qson rates file structured for minimal parsing effort
	# split by object and array delimiters to get instrument blobs
	my @data = grep { $_ =~ /instrument/ } (split(/[{}]/, $response->decoded_content(charset => 'utf-8')));
	#  normalization of whitespace
	s/(^[ \t\r\n]+)|([ \t\r\n]+$)|([ \t]{2,})//g foreach (@data);
	# special whitespace
	s/[\r\n]+/@/g foreach (@data);
	open RATESX, '>'.$rates_file or die "$_sub: open: $!";
	print RATESX join("\n", @data);
	close RATESX;
}

do_instruments();
do_rates();

