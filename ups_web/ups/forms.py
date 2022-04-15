import imp
from django.contrib.auth.forms import UserCreationForm
from django import forms
from django.contrib.auth.models import User
from .models import Account

class RegisterForm(UserCreationForm):
    email = forms.EmailField()
    first_name = forms.CharField(max_length=30)
    last_name = forms.CharField(max_length=50)

    class Meta:
        model = User
        fields = ['username', 'first_name', 'last_name', 'email',
                  'password1', 'password2']

class AccountForm(forms.ModelForm):
    accountid = forms.IntegerField(min_value=0)
    username = forms.CharField(max_length=40)
    password = forms.CharField(max_length=40)

    class Meta:
        model = Account
        fields = ['accountid', 'username', 'password']