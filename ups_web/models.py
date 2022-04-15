# This is an auto-generated Django model module.
# You'll have to do the following manually to clean this up:
#   * Rearrange models' order
#   * Make sure each model has one field with primary_key=True
#   * Make sure each ForeignKey and OneToOneField has `on_delete` set to the desired behavior
#   * Remove `managed = False` lines if you wish to allow Django to create, modify, and delete the table
# Feel free to rename the models, but don't rename db_table values or field names.
from django.db import models


class Account(models.Model):
    accountid = models.AutoField(primary_key=True)
    username = models.CharField(max_length=40)
    password = models.CharField(max_length=40)

    class Meta:
        db_table = 'account'


class Items(models.Model):
    itemid = models.AutoField(primary_key=True)
    description = models.CharField(max_length=2000)
    amount = models.IntegerField()
    packageid = models.ForeignKey('Packages', models.DO_NOTHING, db_column='packageid')

    class Meta:
        db_table = 'items'


class Packages(models.Model):
    packageid = models.AutoField(primary_key=True)
    destx = models.IntegerField(blank=True, null=True)
    desty = models.IntegerField(blank=True, null=True)
    truckid = models.ForeignKey('Trucks', models.DO_NOTHING, db_column='truckid', blank=True, null=True)
    accountid = models.ForeignKey(Account, models.DO_NOTHING, db_column='accountid', blank=True, null=True)
    warehouseid = models.IntegerField()
    status = models.TextField()  # This field type is a guess.

    class Meta:
        db_table = 'packages'


class Trucks(models.Model):
    truckid = models.AutoField(primary_key=True)
    status = models.TextField()  # This field type is a guess.
    x = models.IntegerField()
    y = models.IntegerField()

    class Meta:
        db_table = 'trucks'
