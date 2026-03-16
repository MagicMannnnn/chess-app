import 'react-native-reanimated'

import { DarkTheme, DefaultTheme, ThemeProvider } from '@react-navigation/native'
import { Stack } from 'expo-router'
import * as SplashScreen from 'expo-splash-screen'
import { useEffect } from 'react'
import { SafeAreaProvider } from 'react-native-safe-area-context'

import { useColorScheme } from '@/components/useColorScheme'
import { SettingsProvider } from '@/contexts/SettingsContext'

export { ErrorBoundary } from 'expo-router'

export const unstable_settings = {
  initialRouteName: '(tabs)',
}

SplashScreen.preventAutoHideAsync()

export default function RootLayout() {
  useEffect(() => {
    SplashScreen.hideAsync()
  }, [])

  return <RootLayoutNav />
}

function RootLayoutNav() {
  const colorScheme = useColorScheme()

  return (
    <SafeAreaProvider>
      <SettingsProvider>
        <ThemeProvider value={colorScheme === 'dark' ? DarkTheme : DefaultTheme}>
          <Stack>
            <Stack.Screen
              name="(tabs)"
              options={{
                headerShown: false,
                gestureEnabled: false,
              }}
            />
            <Stack.Screen name="settings" options={{ presentation: 'modal', title: 'Settings' }} />
          </Stack>
        </ThemeProvider>
      </SettingsProvider>
    </SafeAreaProvider>
  )
}
