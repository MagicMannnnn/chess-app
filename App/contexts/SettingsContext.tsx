import React, { createContext, ReactNode, useContext, useEffect, useState } from 'react'
import AsyncStorage from '@react-native-async-storage/async-storage'

interface AppSettings {
  autoFlipBoard: boolean
  aiVersion: 'v1' | 'v2'
}

interface SettingsContextType {
  settings: AppSettings
  updateSettings: (newSettings: Partial<AppSettings>) => void
}

const defaultSettings: AppSettings = {
  autoFlipBoard: false,
  aiVersion: 'v1',
}

const SETTINGS_STORAGE_KEY = '@chess_app_settings'

const SettingsContext = createContext<SettingsContextType | undefined>(undefined)

export function SettingsProvider({ children }: { children: ReactNode }) {
  const [settings, setSettings] = useState<AppSettings>(defaultSettings)
  const [isLoaded, setIsLoaded] = useState(false)

  // Load settings from storage on mount
  useEffect(() => {
    const loadSettings = async () => {
      try {
        const storedSettings = await AsyncStorage.getItem(SETTINGS_STORAGE_KEY)
        if (storedSettings) {
          const parsed = JSON.parse(storedSettings)
          setSettings({ ...defaultSettings, ...parsed })
          console.log('Settings loaded:', parsed)
        }
      } catch (error) {
        console.error('Failed to load settings:', error)
      } finally {
        setIsLoaded(true)
      }
    }
    loadSettings()
  }, [])

  const updateSettings = async (newSettings: Partial<AppSettings>) => {
    const updated = { ...settings, ...newSettings }
    setSettings(updated)

    // Persist to storage
    try {
      await AsyncStorage.setItem(SETTINGS_STORAGE_KEY, JSON.stringify(updated))
      console.log('Settings saved:', updated)
    } catch (error) {
      console.error('Failed to save settings:', error)
    }
  }

  // Don't render children until settings are loaded
  if (!isLoaded) {
    return null
  }

  return (
    <SettingsContext.Provider value={{ settings, updateSettings }}>
      {children}
    </SettingsContext.Provider>
  )
}

export function useSettings() {
  const context = useContext(SettingsContext)
  if (!context) {
    throw new Error('useSettings must be used within a SettingsProvider')
  }
  return context
}
